///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2013 Rick Taylor
//
// This file is part of OOSvrReg, the Omega Online registry server.
//
// OOSvrReg is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrReg is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOSvrReg.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer_Registry.h"
#include "RegistryRootConn.h"
#include "RegistryManager.h"

Registry::RootConnection::RootConnection(Manager* pManager, OOBase::RefPtr<OOBase::AsyncSocket>& sock) :
		m_pManager(pManager),
		m_socket(sock)
{
}

Registry::RootConnection::~RootConnection()
{
	// Tell the manager that we have closed
	m_pManager->quit();
}

bool Registry::RootConnection::start()
{
	// Read start-up params
	OOBase::CDRStream stream;
	int err = OOBase::CDRIO::recv_with_header_blocking<uint16_t>(stream,m_socket);
	if (err)
		LOG_ERROR_RETURN(("Failed to read root request: %s",OOBase::system_error_text(err)),false);

	uint16_t response_id = 0;
	stream.read(response_id);

	// Read DB name and root settings
	OOBase::StackAllocator<512> allocator;
	OOBase::LocalString strDb(allocator);
	stream.read_string(strDb);

	uint8_t nThreads = 0;
	stream.read(nThreads);

	OOBase::Table<OOBase::LocalString,OOBase::LocalString,OOBase::AllocatorInstance> tabSettings(allocator);
	for (;;)
	{
		OOBase::LocalString k(allocator),v(allocator);
		if (!stream.read_string(k) || k.empty() || !stream.read_string(v))
			break;

		err = tabSettings.insert(k,v);
		if (err)
			LOG_ERROR_RETURN(("Failed to insert setting into table: %s",OOBase::system_error_text(err)),false);
	}
	err = stream.last_error();
	if (err)
		LOG_ERROR_RETURN(("Failed to read root request: %s",OOBase::system_error_text(err)),false);

	int ret_err = m_pManager->on_start(strDb,nThreads,tabSettings);

	stream.reset();
	
	size_t mark = stream.buffer()->mark_wr_ptr();

	stream.write(uint16_t(0));
	stream.write(response_id);
	stream.write(static_cast<int32_t>(ret_err));

	stream.replace(static_cast<uint16_t>(stream.length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())),false);

	err = m_socket->send(stream.buffer());
	if (err)
		LOG_ERROR_RETURN(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())),false);

	return recv_next();
}

bool Registry::RootConnection::recv_next()
{
#if defined(_WIN32)

	addref();

	int err = OOBase::CDRIO::recv_with_header_sync<uint16_t>(128,m_socket,this,&RootConnection::on_message_win32);
#elif defined(HAVE_UNISTD_H)
	OOBase::RefPtr<OOBase::Buffer> ctl_buffer = OOBase::Buffer::create(CMSG_SPACE(sizeof(int)),sizeof(size_t));
	if (!ctl_buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	addref();

	int err = OOBase::CDRIO::recv_msg_with_header_sync<uint16_t>(128,m_socket,this,&RootConnection::on_message_posix,ctl_buffer);
#endif
	if (err)
	{
		release();
		LOG_ERROR_RETURN(("Failed to receive from root: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

#if defined(_WIN32)

void Registry::RootConnection::on_message_win32(OOBase::CDRStream& stream, int err)
{
	if (stream.length() == 0)
		LOG_ERROR(("Root pipe disconnected"));
	else if (err)
		LOG_ERROR(("Failed to receive from root pipe: %s",OOBase::system_error_text(err)));
	else
		on_message(stream);

	release();
}

void Registry::RootConnection::PipeConnection::onWait(void* param, HANDLE hObject, bool /*bTimedout*/, int err)
{
	PipeConnection* pThis = static_cast<PipeConnection*>(param);
	OOBase::Win32::SmartHandle hProcess(hObject);

	if (err != ERROR_CANCELLED)
	{
		if (err)
			LOG_ERROR(("Failed to wait for user process: %s",OOBase::system_error_text(err)));
		else
			LOG_WARNING(("User process died before connecting to unique pipe"));

		// Close the pipe, the process has died
		pThis->m_pipe = NULL;
	
		// Remove us from the waiting list
		OOBase::Guard<OOBase::SpinLock> guard(pThis->m_parent->m_lock);
		pThis->m_parent->m_mapConns.remove(pThis);
		guard.release();
	}

	// Done here
	pThis->m_parent->release();
	pThis->release();
}

void Registry::RootConnection::PipeConnection::onAccept(void* param, OOBase::AsyncSocket* pSocket, int err)
{
	PipeConnection* pThis = static_cast<PipeConnection*>(param);
	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket(pSocket);

	if (err)
		LOG_ERROR(("User process pipe accept failed: %s",OOBase::system_error_text(err)));
	else
	{
		HANDLE hPipe = (HANDLE)pSocket->get_handle();

		if (!ImpersonateNamedPipeClient(hPipe))
			LOG_ERROR(("ImpersonateNamedPipeClient failed: %s",OOBase::system_error_text()));
		else
		{
			uid_t uid;
			BOOL bRes = OpenThreadToken(GetCurrentThread(),TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,FALSE,&uid);
			if (!bRes)
				err = GetLastError();

			if (!RevertToSelf())
				OOBase_CallCriticalFailure(GetLastError());
			
			if (!err)
				pThis->m_parent->m_pManager->new_connection(ptrSocket,uid);
		}
	}

	// Close the wait, something happened on the pipe
	pThis->m_wait = NULL;

	// Remove us from the waiting list
	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_parent->m_lock);
	pThis->m_parent->m_mapConns.remove(pThis);
	guard.release();

	// Done here
	pThis->m_parent->release();
	pThis->release();
}

int Registry::RootConnection::PipeConnection::start(DWORD pid, OOBase::LocalString& strPipe, const char* pszSID)
{
	OOBase::Win32::SmartHandle hProcess(OpenProcess(SYNCHRONIZE,FALSE,pid));
	if (!hProcess)
		return GetLastError();

	addref();
	m_parent->addref();

	int err = 0;
	m_wait = m_parent->m_pManager->m_proactor->wait_for_object(this,&onWait,hProcess,err);
	if (err)
	{
		LOG_ERROR(("Failed to wait on process handle: %s",OOBase::system_error_text(err)));
		release();
		m_parent->release();
		return err;
	}

	hProcess.detach();

	addref();
	m_parent->addref();

	char szPipe[64] = {0};
	m_pipe = m_parent->m_pManager->m_proactor->accept_unique_pipe(this,&onAccept,szPipe,err,pszSID);
	if (err)
	{
		LOG_ERROR(("Failed to create unique pipe: %s",OOBase::system_error_text(err)));
		release();
		m_parent->release();
		m_wait = NULL;
		return err;
	}

	err = strPipe.assign(szPipe);
	if (err)
	{
		LOG_ERROR(("Failed to assign string: %s",OOBase::system_error_text(err)));
		m_pipe = NULL;
		m_wait = NULL;
	}

	return err;
}

void Registry::RootConnection::new_connection(OOBase::CDRStream& stream)
{
	uint16_t response_id = 0;
	stream.read(response_id);

	pid_t pid = 0;
	stream.read(pid);

	OOBase::StackAllocator<256> allocator;
	OOBase::LocalString strSID(allocator);
	stream.read_string(strSID);

	if (stream.last_error())
		LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
	else
	{
		stream.reset();
		
		size_t mark = stream.buffer()->mark_wr_ptr();

		stream.write(uint16_t(0));
		stream.write(response_id);

		int ret_err = 0;
		OOBase::LocalString strPipe(allocator);

		PipeConnection* pConn = NULL;
		if (!OOBase::CrtAllocator::allocate_new(pConn,this))
		{
			ret_err = ERROR_OUTOFMEMORY;
			LOG_ERROR(("Failed to allocate connection: %s",OOBase::system_error_text(ret_err)));
		}
		else
		{
			OOBase::RefPtr<PipeConnection> ptrConn(pConn);

			OOBase::Guard<OOBase::SpinLock> guard(m_lock);

			ret_err = m_mapConns.insert(pConn,ptrConn);
			if (ret_err)
				LOG_ERROR(("Failed to insert connection: %s",OOBase::system_error_text(ret_err)));
			else
			{
				ret_err = ptrConn->start(pid,strPipe,strSID.c_str());
				if (ret_err)
				{
					m_mapConns.remove(pConn);
					LOG_ERROR(("Failed to start connection: %s",OOBase::system_error_text(ret_err)));
				}
			}
		}

		stream.write(static_cast<int32_t>(ret_err));
		if (!ret_err)
			stream.write_string(strPipe);

		stream.replace(static_cast<uint16_t>(stream.length()),mark);
		if (stream.last_error())
			LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
		else
		{
			addref();

			int err = m_socket->send(this,&RootConnection::on_sent,stream.buffer());
			if (err)
			{
				LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())));

				release();
			}
		}
	}
}

#elif defined(HAVE_UNISTD_H)

void Registry::RootConnection::on_message_posix(OOBase::CDRStream& stream, OOBase::Buffer* ctl_buffer, int err)
{
	if (stream.length() == 0)
		LOG_ERROR(("Root pipe disconnected"));
	else if (err)
		LOG_ERROR(("Failed to receive from root pipe: %s",OOBase::system_error_text(err)));
	else
	{
		OOBase::POSIX::SmartFD passed_fd;

		// Read struct cmsg
		struct msghdr msgh = {0};
		msgh.msg_control = const_cast<char*>(ctl_buffer->rd_ptr());
		msgh.msg_controllen = ctl_buffer->length();

		for (struct cmsghdr* msg = CMSG_FIRSTHDR(&msgh);msg;msg = CMSG_NXTHDR(&msgh,msg))
		{
			if (msg->cmsg_level == SOL_SOCKET && msg->cmsg_type == SCM_RIGHTS)
			{
				int* fds = reinterpret_cast<int*>(CMSG_DATA(msg));
				size_t fd_count = (msg->cmsg_len - CMSG_LEN(0))/sizeof(int);
				size_t fd_start = 0;
				if (fd_count > 0)
				{
					err = OOBase::POSIX::set_close_on_exec(fds[0],true);
					if (!err)
					{
						passed_fd = fds[0];
						fd_start = 1;
					}
				}

				for (size_t i=fd_start;i<fd_count;++i)
					OOBase::POSIX::close(fds[i]);
			}
			else
			{
				LOG_ERROR(("Root pipe control data has weird stuff in it"));
				err = EINVAL;
			}
		}

		if (!passed_fd.is_valid())
		{
			err = EINVAL;
			LOG_ERROR(("Root pipe control data invalid handle"));
		}

		if (!err)
			on_message(stream,passed_fd);
	}

	release();
}

void Registry::RootConnection::new_connection(OOBase::CDRStream& stream, OOBase::POSIX::SmartFD& passed_fd)
{
	uint16_t response_id;
	stream.read(response_id);
	uid_t uid;
	stream.read(uid);

	if (stream.last_error())
		LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
	else
	{
		// Attach to the pipe
		int ret_err = 0;
		OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket = m_pManager->m_proactor->attach(passed_fd,ret_err);
		if (ret_err)
			LOG_ERROR(("Failed to attach to user pipe: %s",OOBase::system_error_text(ret_err)));
		else
		{
			passed_fd.detach();

			// Tell the manager to create a new connection
			ret_err = m_pManager->new_connection(ptrSocket,uid);
		}

		stream.reset();

		size_t mark = stream.buffer()->mark_wr_ptr();

		stream.write(uint16_t(0));
		stream.write(response_id);
		stream.write(static_cast<int32_t>(ret_err));

		stream.replace(static_cast<uint16_t>(stream.length()),mark);
		if (stream.last_error())
			LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
		else
		{
			addref();

			int err = m_socket->send(this,&RootConnection::on_sent,stream.buffer());
			if (err)
			{
				LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())));

				release();
			}
		}
	}
}

#endif

void Registry::RootConnection::on_sent(OOBase::Buffer* /*buffer*/, int err)
{
	if (err)
		LOG_ERROR(("Failed to send data to root: %s",OOBase::system_error_text(err)));

	release();
}

#if defined(_WIN32)
void Registry::RootConnection::on_message(OOBase::CDRStream& stream)
#elif defined(HAVE_UNISTD_H)
void Registry::RootConnection::on_message(OOBase::CDRStream& stream, OOBase::POSIX::SmartFD& passed_fd)
#endif
{
	OOServer::Root2Reg_OpCode_t op_code;
	if (!stream.read(op_code))
		LOG_ERROR(("Failed to read request opcode from root: %s",OOBase::system_error_text(stream.last_error())));
	else
	{
		switch (static_cast<OOServer::Root2Reg_OpCode>(op_code))
		{
		case OOServer::Root2Reg_NewConnection:
			if (recv_next())
			{
#if defined(_WIN32)
				new_connection(stream);
#elif defined(HAVE_UNISTD_H)
				new_connection(stream,passed_fd);
#endif
			}
			break;

		default:
			break;
		}
	}
}
