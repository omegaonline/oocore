///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2013 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer_User.h"
#include "UserRootConn.h"
#include "UserManager.h"

#include <stdlib.h>

User::RootConnection::RootConnection(Manager* pManager, OOBase::RefPtr<OOBase::AsyncSocket>& sock) :
		m_pManager(pManager),
		m_socket(sock)
{
}

User::RootConnection::~RootConnection()
{
	// Tell the manager that we have closed
	m_pManager->quit();
}

#if defined(_WIN32)

bool User::RootConnection::start()
{
	OOBase::CDRStream stream;
	int err = OOBase::CDRIO::recv_with_header_blocking<Omega::uint16_t>(stream,m_socket);
	if (err)
		LOG_ERROR_RETURN(("Failed to receive from root pipe: %s",OOBase::system_error_text(err)),false);

	Omega::uint16_t response_id = 0;
	stream.read(response_id);

	OOBase::StackAllocator<256> allocator;
	OOBase::LocalString strUserPipe(allocator),strRootPipe(allocator);
	stream.read_string(strUserPipe);
	stream.read_string(strRootPipe);

	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())),false);

	// Align everything, ready for the Omega_Initialize call
	stream.buffer()->align_rd_ptr(OOBase::CDRStream::MaxAlignment);

	int ret_err = 0;
	OOBase::Timeout timeout(20,0);
	OOBase::RefPtr<OOBase::AsyncSocket> ptrUserSocket = m_pManager->m_proactor->connect(strUserPipe.c_str(),ret_err,timeout);
	if (ret_err)
		LOG_ERROR(("Failed to connect to registry pipe: %s",OOBase::system_error_text(ret_err)));
	else
	{
		OOBase::RefPtr<OOBase::AsyncSocket> ptrRootSocket;
		if (!strRootPipe.empty())
		{
			ptrRootSocket = m_pManager->m_proactor->connect(strRootPipe.c_str(),ret_err,timeout);
			if (ret_err)
				LOG_ERROR(("Failed to connect to registry pipe: %s",OOBase::system_error_text(ret_err)));
		}

		if (!ret_err)
			ret_err = m_pManager->start(ptrUserSocket,ptrRootSocket,stream);
	}

	stream.reset();
	size_t mark = stream.buffer()->mark_wr_ptr();
	stream.write(Omega::uint16_t(0));
	stream.write(response_id);
	stream.write(static_cast<Omega::int32_t>(ret_err));

	stream.replace(static_cast<Omega::uint16_t>(stream.length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())),false);

	err = m_socket->send(stream.buffer());
	if (err)
		LOG_ERROR_RETURN(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())),false);

	return recv_next();
}

void User::RootConnection::on_message_win32(OOBase::CDRStream& stream, int err)
{
	if (stream.length() == 0)
		LOG_ERROR(("Root pipe disconnected"));
	else if (err)
		LOG_ERROR(("Failed to receive from root pipe: %s",OOBase::system_error_text(err)));
	else
		on_message(stream);

	release();
}

void User::RootConnection::new_connection(OOBase::CDRStream& stream)
{
	// Read and cache any root parameters
	Omega::uint16_t response_id = 0;
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

		stream.write(Omega::uint16_t(0));
		stream.write(response_id);

		OOBase::LocalString pipe(allocator);

		int ret_err = 0;
		void* TODO; // TODO, Omega_ConnectChannel(passed_fd,pid) ?!?!

		stream.write(static_cast<Omega::int32_t>(ret_err));
		if (!ret_err)
			stream.write_string(pipe);

		stream.replace(static_cast<Omega::uint16_t>(stream.length()),mark);
		if (stream.last_error())
			LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
		else
		{
			addref();

			int err = m_socket->send(this,&RootConnection::on_sent,stream.buffer());
			if (err)
			{
				release();

				LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(err)));
			}
		}
	}
}

#elif defined(HAVE_UNISTD_H)

bool User::RootConnection::start()
{
	OOBase::CDRStream stream;
	OOBase::RefPtr<OOBase::Buffer> ctl_buffer = OOBase::Buffer::create(2*CMSG_SPACE(sizeof(int)),sizeof(size_t));
	if (!ctl_buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	int err = OOBase::CDRIO::recv_msg_with_header_blocking<Omega::uint16_t>(stream,ctl_buffer,m_socket);
	if (err)
		LOG_ERROR_RETURN(("Failed to recv from root pipe: %s",OOBase::system_error_text(err)),false);

	// User, Root, Sandbox
	OOBase::POSIX::SmartFD passed_fds[3];

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
			for (size_t i=0;i<fd_count && i < (sizeof(passed_fds)/sizeof(passed_fds[0]));++i)
			{
				err = OOBase::POSIX::set_close_on_exec(fds[i],true);
				if (!err)
				{
					passed_fds[i] = fds[i];
					++fd_start;
				}
			}

			for (size_t i=fd_start;i<fd_count;++i)
				OOBase::POSIX::close(fds[i]);
		}
		else
			LOG_ERROR_RETURN(("Root pipe control data has weird stuff in it"),false);
	}

	if (!passed_fds[0].is_valid())
		LOG_ERROR_RETURN(("Root pipe control data invalid handle"),false);

	Omega::uint16_t response_id = 0;
	if (!stream.read(response_id))
		LOG_ERROR_RETURN(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())),false);

	// Align everything, ready for the Omega_Initialize call
	stream.buffer()->align_rd_ptr(OOBase::CDRStream::MaxAlignment);

	// Add the extra fd to the stream
	if (!stream.write(static_cast<int>(passed_fds[2])))
		LOG_ERROR_RETURN(("Failed to write request from root: %s",OOBase::system_error_text(stream.last_error())),false);

	// Attach to the pipe
	int ret_err = 0;
	OOBase::RefPtr<OOBase::AsyncSocket> ptrUserSocket = m_pManager->m_proactor->attach(passed_fds[0],ret_err);
	if (ret_err)
		LOG_ERROR(("Failed to attach to registry pipe: %s",OOBase::system_error_text(ret_err)));
	else
	{
		passed_fds[0].detach();

		OOBase::RefPtr<OOBase::AsyncSocket> ptrRootSocket;
		if (passed_fds[1].is_valid())
		{
			ptrRootSocket = m_pManager->m_proactor->attach(passed_fds[1],ret_err);
			if (ret_err)
				LOG_ERROR(("Failed to attach to registry pipe: %s",OOBase::system_error_text(ret_err)));
			else
				passed_fds[1].detach();
		}

		if (!ret_err)
		{
			// Tell the manager to create a new connection
			ret_err = m_pManager->start(ptrUserSocket,ptrRootSocket,stream);
		}
	}

	stream.reset();

	size_t mark = stream.buffer()->mark_wr_ptr();

	stream.write(Omega::uint16_t(0));
	stream.write(response_id);
	stream.write(static_cast<Omega::int32_t>(ret_err));

	stream.replace(static_cast<Omega::uint16_t>(stream.length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())),false);

	err = m_socket->send(stream.buffer());
	if (err)
		LOG_ERROR_RETURN(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())),false);

	return recv_next();
}

void User::RootConnection::on_message_posix(OOBase::CDRStream& stream, OOBase::Buffer* ctl_buffer, int err)
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

void User::RootConnection::new_connection(OOBase::CDRStream& stream, OOBase::POSIX::SmartFD& passed_fd)
{
	// Read and cache any root parameters
	Omega::uint16_t response_id = 0;
	stream.read(response_id);
	pid_t pid;
	stream.read(pid);

	if (stream.last_error())
		LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
	else
	{
		LOG_DEBUG(("New user connection requested, pid %u",pid));

		stream.reset();

		size_t mark = stream.buffer()->mark_wr_ptr();

		stream.write(Omega::uint16_t(0));
		stream.write(response_id);

		void* TODO; // TODO, Omega_ConnectChannel(passed_fd,pid) ?!?!
		int ret_err = 0;

		if (ret_err)
			passed_fd.detach();

		stream.write(static_cast<Omega::int32_t>(ret_err));
		stream.replace(static_cast<Omega::uint16_t>(stream.length()),mark);
		if (stream.last_error())
			LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
		else
		{
			addref();

			int err = m_socket->send(this,&RootConnection::on_sent,stream.buffer());
			if (err)
			{
				release();

				LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())));
			}
		}
	}
}

#endif

void User::RootConnection::on_sent(OOBase::Buffer* /*buffer*/, int err)
{
	if (err)
		LOG_ERROR(("Failed to send data to root: %s",OOBase::system_error_text(err)));

	release();
}

bool User::RootConnection::recv_next()
{
#if defined(_WIN32)

	addref();

	int err = OOBase::CDRIO::recv_with_header_sync<Omega::uint16_t>(128,m_socket,this,&RootConnection::on_message_win32);
#elif defined(HAVE_UNISTD_H)
	OOBase::RefPtr<OOBase::Buffer> ctl_buffer = OOBase::Buffer::create(CMSG_SPACE(sizeof(int)),sizeof(size_t));
	if (!ctl_buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	addref();

	int err = OOBase::CDRIO::recv_msg_with_header_sync<Omega::uint16_t>(128,m_socket,this,&RootConnection::on_message_posix,ctl_buffer);
#endif
	if (err)
	{
		release();
		LOG_ERROR_RETURN(("Failed to receive from root: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

bool User::Manager::connect_root(const OOBase::LocalString& strPipe)
{
	int err = 0;

#if defined(_WIN32)
	// Use a named pipe
	OOBase::Timeout timeout(20,0);
	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket = m_proactor->connect(strPipe.c_str(),err,timeout);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to connect to root pipe: %s",OOBase::system_error_text(err)),false);

#else

	// Use the passed fd
	int fd = atoi(strPipe.c_str());
	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket = m_proactor->attach(fd,err);
	if (err != 0)
	{
		OOBase::POSIX::close(fd);
		LOG_ERROR_RETURN(("Failed to attach to root pipe: %s",OOBase::system_error_text(err)),false);
	}

#endif

	m_root_connection = new User::RootConnection(this,ptrSocket);
	if (!m_root_connection)
		LOG_ERROR_RETURN(("Failed to create root connection: %s",OOBase::system_error_text()),false);

	return m_root_connection->start();
}

#if defined(_WIN32)
void User::RootConnection::on_message(OOBase::CDRStream& stream)
#elif defined(HAVE_UNISTD_H)
void User::RootConnection::on_message(OOBase::CDRStream& stream, OOBase::POSIX::SmartFD& passed_fd)
#endif
{
	OOServer::Root2User_OpCode_t op_code;
	if (!stream.read(op_code))
		LOG_ERROR(("Failed to read request opcode from root: %s",OOBase::system_error_text(stream.last_error())));
	else
	{
		switch (static_cast<OOServer::Root2User_OpCode>(op_code))
		{
		case OOServer::Root2User_NewConnection:
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

/*void User::Manager::process_root_request(OOBase::CDRStream& request, uint16_t src_thread_id, uint32_t attribs)
{
	OOServer::RootOpCode_t op_code;
	if (!request.read(op_code))
	{
		LOG_ERROR(("Bad request: %s",OOBase::system_error_text(request.last_error())));
		return;
	}

	OOBase::CDRStream response;
	switch (op_code)
	{
	case OOServer::Service_Start:
		start_service(request,attribs & OOServer::Message_t::asynchronous ? NULL : &response);
		break;

	case OOServer::Service_Stop:
		stop_service(request,response);
		break;

	case OOServer::Service_StopAll:
		stop_all_services(response);
		break;

	case OOServer::Service_IsRunning:
		service_is_running(request,response);
		break;

	case OOServer::Service_ListRunning:
		list_services(response);
		break;

	default:
		response.write(static_cast<OOServer::RootErrCode_t>(OOServer::Errored));
		LOG_ERROR(("Bad request op_code: %u",op_code));
		break;
	}

	if (!response.last_error() && !(attribs & OOServer::Message_t::asynchronous))
	{
		OOServer::MessageHandler::io_result::type res = send_response(m_uUpstreamChannel,src_thread_id,response,attribs);
		if (res == OOServer::MessageHandler::io_result::failed)
			LOG_ERROR(("Root response sending failed"));
	}
}*/
