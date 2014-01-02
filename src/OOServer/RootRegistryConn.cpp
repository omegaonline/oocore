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

#include "OOServer_Root.h"
#include "RootManager.h"

#include <stdlib.h>

Root::RegistryConnection::RegistryConnection(Manager* pManager, OOBase::SharedPtr<Process>& ptrProcess, OOBase::RefPtr<OOBase::AsyncSocket>& ptrSocket) :
	m_pManager(pManager),
	m_ptrProcess(ptrProcess),
	m_ptrSocket(ptrSocket),
	m_id(0)
{
}

bool Root::RegistryConnection::start(size_t id)
{
	m_id = id;

	OOBase::RefPtr<OOBase::Buffer> buffer = OOBase::Buffer::create(256,OOBase::CDRStream::MaxAlignment);
	if (!buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	addref();

	int err = m_ptrSocket->recv(this,&RegistryConnection::on_response,buffer);
	if (err)
	{
		release();

		LOG_ERROR_RETURN(("Failed to start response io: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

void Root::RegistryConnection::on_response(OOBase::Buffer* buffer, int err)
{
	if (err)
		LOG_ERROR(("Failed to receive from user registry: %s",OOBase::system_error_text(err)));
	else if (buffer->length() == 0)
	{
		LOG_WARNING(("Registry process %lu has closed",(unsigned long)m_ptrProcess->get_pid()));
		err = -1;
	}

	while (!err && buffer->length() >= sizeof(OOBase::uint16_t))
	{
		OOBase::CDRStream response(buffer);

		OOBase::uint16_t len = 0;
		if (!response.read(len))
		{
			err = response.last_error();
			LOG_ERROR(("Failed to receive from user registry: %s",OOBase::system_error_text(err)));
		}
		else if (buffer->length() < len - sizeof(OOBase::uint16_t))
		{
			// Read the rest of the message
			err = m_ptrSocket->recv(this,&RegistryConnection::on_response,buffer,len);
			if (!err)
				return;

			LOG_ERROR(("Failed to continue response io: %s",OOBase::system_error_text(err)));
		}
		else
		{
			if (!m_async_dispatcher.handle_response(response))
			{
				LOG_ERROR(("Invalid or missing response"));
				err = -1;
			}
			else
				buffer->compact();
		}
	}

	if (!err)
	{
		err = m_ptrSocket->recv(this,&RegistryConnection::on_response,buffer);
		if (err)
			LOG_ERROR(("Failed to continue response io: %s",OOBase::system_error_text(err)));
	}

	if (err)
	{
		m_pManager->drop_registry_process(m_id);

		release();
	}
}

void Root::RegistryConnection::on_sent(OOBase::Buffer* buffer, int err)
{
	if (err)
	{
		LOG_ERROR(("Failed to send data to root: %s",OOBase::system_error_text(err)));

		buffer->mark_rd_ptr(0);

		OOBase::CDRStream stream(buffer);

		OOBase::uint16_t len = 0;
		OOBase::uint16_t trans_id = 0;
		if (stream.read(len) && stream.read(trans_id) && trans_id)
			m_async_dispatcher.drop_response(trans_id);

		m_pManager->drop_registry_process(m_id);
	}

	release();
}

bool Root::RegistryConnection::start(const OOBase::String& strRegPath, OOBase::RefPtr<ClientConnection>& ptrClient, size_t id)
{
	if (!start(id))
		return false;

	OOBase::AsyncResponseDispatcher<OOBase::uint16_t>::AutoDrop response_id(m_async_dispatcher);
	int err = m_async_dispatcher.add_response(this,&RegistryConnection::on_started,ptrClient->get_pid(),response_id);
	if (err)
		LOG_ERROR_RETURN(("Failed to add async response: %s",OOBase::system_error_text(err)),false);

	// Write initial message
	OOBase::CDRStream stream;
	size_t mark = stream.buffer()->mark_wr_ptr();
	stream.write(OOBase::uint16_t(0));
	response_id.write(stream);
	stream.write_string(strRegPath);

	OOBase::String strThreads;
	m_pManager->get_config_arg("registry_concurrency",strThreads);
	OOBase::uint8_t threads = static_cast<OOBase::uint8_t>(strtoul(strThreads.c_str(),NULL,10));
	if (threads < 1 || threads > 8)
		threads = 2;

	stream.write(threads);
	stream.write("");

	stream.replace(static_cast<OOBase::uint16_t>(stream.length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	addref();

	// Send the message
	err = m_ptrSocket->send(this,&RegistryConnection::on_sent,stream.buffer());
	if (err)
	{
		release();

		LOG_ERROR_RETURN(("Failed to send registry start data: %s",OOBase::system_error_text(err)),false);
	}

	response_id.detach();
	return true;
}

bool Root::RegistryConnection::on_started(OOBase::CDRStream& stream, pid_t client_id)
{
	bool bSuccess = false;

	ClientConnection::AutoDrop drop(m_pManager,client_id);

	OOBase::int32_t ret_err = 0;
	if (!stream.read(ret_err))
		LOG_ERROR(("Failed to read start response from registry: %s",OOBase::system_error_text(stream.last_error())));
	else if (ret_err)
		LOG_ERROR(("Registry failed to start properly: %s",OOBase::system_error_text(ret_err)));
	else
	{
		OOBase::RefPtr<RegistryConnection> ptrThis = this;
		ptrThis->addref();

		if (m_pManager->spawn_user_process(client_id,ptrThis))
		{
			drop.detach();
			bSuccess = true;
			OOBase::Logger::log(OOBase::Logger::Information,"User registry started successfully");
		}
	}

	if (!bSuccess)
		m_pManager->drop_registry_process(m_id);

	return bSuccess;
}

bool Root::RegistryConnection::same_user(const uid_t& uid) const
{
	return m_ptrProcess->same_user(uid);
}

bool Root::RegistryConnection::start_user(OOBase::RefPtr<UserConnection>& ptrUser)
{
	OOBase::RefPtr<ClientConnection> ptrClient;
	return start_user(ptrClient,ptrUser);
}

#if defined(_WIN32)

bool Root::RegistryConnection::start_user(OOBase::RefPtr<ClientConnection>& ptrClient, OOBase::RefPtr<UserConnection>& ptrUser)
{
	// Create new connection for the user process
	pid_t client_id = 0;
	if (ptrClient)
		client_id = ptrClient->get_pid();

	OOBase::AsyncResponseDispatcher<OOBase::uint16_t>::AutoDrop response_id(m_async_dispatcher);
	int err = m_async_dispatcher.add_response(this,&RegistryConnection::on_start_user,client_id,ptrUser->get_pid(),response_id);
	if (err)
		LOG_ERROR_RETURN(("Failed to enqueue response: %s",OOBase::system_error_text(err)),false);

	return new_connection(ptrUser,response_id);
}

bool Root::RegistryConnection::on_start_user(OOBase::CDRStream& response, pid_t client_id, pid_t user_id)
{
	ClientConnection::AutoDrop drop1(m_pManager,client_id);

	OOBase::RefPtr<UserConnection> ptrUser;
	if (!m_pManager->get_user_process(user_id,ptrUser))
		return true;

	UserConnection::AutoDrop drop2(m_pManager,user_id);

	OOBase::int32_t ret_err = 0;
	if (!response.read(ret_err))
		LOG_ERROR_RETURN(("Failed to read start user response: %s",OOBase::system_error_text(response.last_error())),true);
	if (ret_err)
		LOG_WARNING_RETURN(("Registry refused user connection: %s",OOBase::system_error_text(ret_err)),true);

	OOBase::String strUserFd;
	if (!response.read_string(strUserFd))
		LOG_ERROR_RETURN(("Failed to read start user response: %s",OOBase::system_error_text(response.last_error())),true);

	if (m_id == 1)
	{
		OOBase::String strRootFd;
		ptrUser->start(client_id,strUserFd,strRootFd);
	}
	else
	{
		OOBase::RefPtr<RegistryConnection> ptrRoot = m_pManager->get_root_registry();
		if (!ptrRoot)
			LOG_ERROR_RETURN(("Failed to get root registry"),false);

		if (!ptrRoot->new_connection2(client_id,ptrUser,strUserFd))
			return false;
	}

	drop1.detach();
	drop2.detach();

	return true;
}

bool Root::RegistryConnection::new_connection2(pid_t client_id, OOBase::RefPtr<UserConnection>& ptrUser, const OOBase::String& strUserFd)
{
	user2_params_t params;
	params.client_id = client_id;
	params.user_id = ptrUser->get_pid();
	params.strUserFd = strUserFd;
	
	OOBase::AsyncResponseDispatcher<OOBase::uint16_t>::AutoDrop response_id(m_async_dispatcher);
	int err = m_async_dispatcher.add_response(this,&RegistryConnection::on_start_user2,params,response_id);
	if (err)
		LOG_ERROR_RETURN(("Failed to enqueue response: %s",OOBase::system_error_text(err)),true);

	return new_connection(ptrUser,response_id);
}

bool Root::RegistryConnection::on_start_user2(OOBase::CDRStream& response, const user2_params_t& params)
{
	ClientConnection::AutoDrop drop1(m_pManager,params.client_id);
	UserConnection::AutoDrop drop2(m_pManager,params.user_id);

	OOBase::RefPtr<UserConnection> ptrUser;
	if (!m_pManager->get_user_process(params.user_id,ptrUser))
		return true;

	OOBase::int32_t ret_err = 0;
	if (!response.read(ret_err))
		LOG_ERROR_RETURN(("Failed to read start user response: %s",OOBase::system_error_text(response.last_error())),true);
	if (ret_err)
		LOG_WARNING_RETURN(("Registry refused user connection: %s",OOBase::system_error_text(ret_err)),true);

	OOBase::String strRootFd;
	if (!response.read_string(strRootFd))
		LOG_ERROR_RETURN(("Failed to read start response: %s",OOBase::system_error_text(response.last_error())),true);

	if (ptrUser->start(params.client_id,params.strUserFd,strRootFd))
	{
		drop1.detach();
		drop2.detach();
	}

	return true;
}

bool Root::RegistryConnection::new_connection(OOBase::RefPtr<UserConnection>& ptrUser, OOBase::AsyncResponseDispatcher<OOBase::uint16_t>::AutoDrop& response_id)
{
	OOBase::StackAllocator<256> allocator;
	OOBase::TempPtr<void> ptrSIDLogon(allocator);
	DWORD dwErr = OOBase::Win32::GetLogonSID(ptrUser->get_uid(),ptrSIDLogon);
	if (dwErr)
		LOG_ERROR_RETURN(("Failed to get logon SID: %s",OOBase::system_error_text(dwErr)),false);

	OOBase::LocalString strSID(allocator);
	dwErr = OOBase::Win32::SIDToString(ptrSIDLogon.get(),strSID);
	if (dwErr)
		LOG_ERROR_RETURN(("Failed to format logon SID: %s",OOBase::system_error_text(dwErr)),false);

	OOBase::CDRStream stream;
	size_t mark = stream.buffer()->mark_wr_ptr();
	stream.write(OOBase::uint16_t(0));

	stream.write(static_cast<OOServer::Root2Reg_OpCode_t>(OOServer::Root2Reg_NewConnection));
	response_id.write(stream);
	stream.write(ptrUser->get_pid());
	stream.write_string(strSID);

	stream.replace(static_cast<OOBase::uint16_t>(stream.length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	addref();

	int err = m_ptrSocket->send(this,&RegistryConnection::on_sent,stream.buffer());
	if (err)
	{
		m_pManager->drop_registry_process(m_id);

		release();

		LOG_ERROR_RETURN(("Failed to send registry data: %s",OOBase::system_error_text(err)),false);
	}

	response_id.detach();
	return true;
}

#elif defined(HAVE_UNISTD_H)

bool Root::RegistryConnection::start_user(OOBase::RefPtr<ClientConnection>& ptrClient, OOBase::RefPtr<UserConnection>& ptrUser)
{
	// Create a pair of sockets
	OOBase::POSIX::SmartFD fds[2];
	int err = OOBase::POSIX::socketpair(SOCK_STREAM,fds);
	if (err)
		LOG_ERROR_RETURN(("socketpair() failed: %s",OOBase::system_error_text(err)),false);

	// Create new connection for the user process
	pid_t client_id = 0;
	if (ptrClient)
		client_id = ptrClient->get_pid();

	OOBase::AsyncResponseDispatcher<OOBase::uint16_t>::AutoDrop response_id(m_async_dispatcher);
	err = m_async_dispatcher.add_response(this,&RegistryConnection::on_start_user,client_id,ptrUser->get_pid(),static_cast<int>(fds[0]),response_id);
	if (err)
		LOG_ERROR_RETURN(("Failed to enqueue response: %s",OOBase::system_error_text(err)),false);

	if (!new_connection(ptrUser,fds[1],response_id))
		return false;

	fds[0].detach();
	return true;
}

bool Root::RegistryConnection::on_start_user(OOBase::CDRStream& response, pid_t client_id, pid_t user_id, int user_fd)
{
	ClientConnection::AutoDrop drop1(m_pManager,client_id);

	OOBase::POSIX::SmartFD ptrUserFd(user_fd);

	OOBase::RefPtr<UserConnection> ptrUser;
	if (!m_pManager->get_user_process(user_id,ptrUser))
		return true;

	UserConnection::AutoDrop drop2(m_pManager,user_id);

	OOBase::int32_t ret_err = 0;
	if (!response.read(ret_err))
		LOG_ERROR_RETURN(("Failed to read start user response: %s",OOBase::system_error_text(response.last_error())),true);
	if (ret_err)
		LOG_WARNING_RETURN(("Registry refused user connection: %s",OOBase::system_error_text(ret_err)),true);

	if (m_id == 1)
	{
		ptrUser->start(client_id,ptrUserFd);
	}
	else
	{
		OOBase::RefPtr<RegistryConnection> ptrRoot = m_pManager->get_root_registry();
		if (!ptrRoot)
			LOG_ERROR_RETURN(("Failed to get root registry"),false);

		if (!ptrRoot->new_connection2(client_id,ptrUser,ptrUserFd))
			return false;
	}

	drop1.detach();
	drop2.detach();

	return true;
}

bool Root::RegistryConnection::new_connection2(pid_t client_id, OOBase::RefPtr<UserConnection>& ptrUser, OOBase::POSIX::SmartFD& ptrUserFd)
{
	// Create a pair of sockets
	OOBase::POSIX::SmartFD fds[2];
	int err = OOBase::POSIX::socketpair(SOCK_STREAM,fds);
	if (err)
		LOG_ERROR_RETURN(("socketpair() failed: %s",OOBase::system_error_text(err)),true);

	user2_params_t params = {0};
	params.client_id = client_id;
	params.user_id = ptrUser->get_pid();
	params.user_fd = static_cast<int>(ptrUserFd);
	params.root_fd = static_cast<int>(fds[0]);
	
	OOBase::AsyncResponseDispatcher<OOBase::uint16_t>::AutoDrop response_id(m_async_dispatcher);
	err = m_async_dispatcher.add_response(this,&RegistryConnection::on_start_user2,params,response_id);
	if (err)
		LOG_ERROR_RETURN(("Failed to enqueue response: %s",OOBase::system_error_text(err)),true);

	OOBase::POSIX::SmartFD fd_root;
	if (!new_connection(ptrUser,fds[1],response_id))
		return false;

	ptrUserFd.detach();
	fds[0].detach();

	return true;
}

bool Root::RegistryConnection::on_start_user2(OOBase::CDRStream& response, const user2_params_t& params)
{
	OOBase::POSIX::SmartFD ptrUserFd(params.user_fd);
	OOBase::POSIX::SmartFD ptrRootFd(params.root_fd);

	ClientConnection::AutoDrop drop1(m_pManager,params.client_id);
	UserConnection::AutoDrop drop2(m_pManager,params.user_id);

	OOBase::RefPtr<UserConnection> ptrUser;
	if (!m_pManager->get_user_process(params.user_id,ptrUser))
		return true;

	OOBase::int32_t ret_err = 0;
	if (!response.read(ret_err))
		LOG_ERROR_RETURN(("Failed to read start user response: %s",OOBase::system_error_text(response.last_error())),true);
	if (ret_err)
		LOG_WARNING_RETURN(("Registry refused user connection: %s",OOBase::system_error_text(ret_err)),true);

	if (ptrUser->start(params.client_id,ptrUserFd,ptrRootFd))
	{
		drop1.detach();
		drop2.detach();
	}

	return true;
}

bool Root::RegistryConnection::new_connection(OOBase::RefPtr<UserConnection>& ptrUser, OOBase::POSIX::SmartFD& fd, OOBase::AsyncResponseDispatcher<OOBase::uint16_t>::AutoDrop& response_id)
{
	OOBase::RefPtr<OOBase::Buffer> ctl_buffer = OOBase::Buffer::create(CMSG_SPACE(sizeof(int)),sizeof(size_t));
	if (!ctl_buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	struct msghdr msg = {0};
	msg.msg_control = ctl_buffer->wr_ptr();
	msg.msg_controllen = ctl_buffer->space();

	struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	*(int*)CMSG_DATA(cmsg) = fd;
	ctl_buffer->wr_ptr(cmsg->cmsg_len);

	OOBase::CDRStream stream;
	size_t mark = stream.buffer()->mark_wr_ptr();
	stream.write(OOBase::uint16_t(0));

	stream.write(static_cast<OOServer::Root2Reg_OpCode_t>(OOServer::Root2Reg_NewConnection));
	response_id.write(stream);
	stream.write(ptrUser->get_uid());

	stream.replace(static_cast<OOBase::uint16_t>(stream.length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	addref();

	int err = m_ptrSocket->send_msg(this,&RegistryConnection::on_sent_msg,stream.buffer(),ctl_buffer);
	if (err)
	{
		m_pManager->drop_registry_process(m_id);

		release();

		LOG_ERROR_RETURN(("Failed to send registry data: %s",OOBase::system_error_text(err)),false);
	}

	fd.detach();
	response_id.detach();

	return true;
}

void Root::RegistryConnection::on_sent_msg(OOBase::Buffer* data, OOBase::Buffer* ctl_buffer, int err)
{
	// Make sure we close all file handles...
	ctl_buffer->mark_rd_ptr(0);

	struct msghdr msgh = {0};
	msgh.msg_control = const_cast<OOBase::uint8_t*>(ctl_buffer->rd_ptr());
	msgh.msg_controllen = ctl_buffer->length();

	for (struct cmsghdr* msg = CMSG_FIRSTHDR(&msgh);msg;msg = CMSG_NXTHDR(&msgh,msg))
	{
		if (msg->cmsg_level == SOL_SOCKET && msg->cmsg_type == SCM_RIGHTS)
		{
			int* fds = reinterpret_cast<int*>(CMSG_DATA(msg));
			size_t fd_count = (msg->cmsg_len - CMSG_LEN(0))/sizeof(int);

			for (size_t i=0;i<fd_count;++i)
				OOBase::POSIX::close(fds[i]);
		}
	}

	on_sent(data,err);
}

#endif

bool Root::RegistryConnection::get_environment(const char* key, OOBase::Environment::env_table_t& tabEnv)
{
	// Get user environment from user registry
	OOBase::CDRStream stream;
	size_t mark = stream.buffer()->mark_wr_ptr();
	stream.write(OOBase::uint16_t(0));

	stream.replace(static_cast<OOBase::uint16_t>(stream.length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	void* TODO; // TODO
	/*if (err)
		LOG_ERROR_RETURN(("Failed to send registry data: %s",OOBase::system_error_text(err)),false);

	OOBase::int32_t ret_err = 0;
	if (!stream.read(ret_err))
		LOG_ERROR_RETURN(("Failed to read registry response from registry: %s",OOBase::system_error_text(stream.last_error())),false);
	if (ret_err)
		LOG_ERROR_RETURN(("Registry failed to respond properly: %s",OOBase::system_error_text(ret_err)),false);

	OOBase::Environment::env_table_t tabEnv(allocator);
	size_t num_vars;
	stream.read(num_vars);
	for (size_t i=0;i<num_vars;++i)
	{
		OOBase::LocalString key(allocator),value(allocator);

		if (!stream.read_string(key) || !stream.read_string(value))
			LOG_ERROR_RETURN(("Registry failed to respond properly: %s",OOBase::system_error_text(stream.last_error())),false);

		err = tabEnv.insert(key,value);
		if (err)
			LOG_ERROR_RETURN(("Failed to insert environment variable into block: %s",OOBase::system_error_text(err)),false);
	}
*/

	return true;
}

bool Root::Manager::start_system_registry()
{
	OOBase::Logger::log(OOBase::Logger::Information,"Starting system registry...");

	// Get dir from config
	OOBase::String strRegPath;
	if (!get_config_arg("regdb_path",strRegPath) || strRegPath.empty())
		LOG_ERROR_RETURN(("Missing 'regdb_path' config setting"),false);

	int err = strRegPath.append("system.regdb");
	if (err)
		LOG_ERROR_RETURN(("Failed to append string: %s",OOBase::system_error_text(err)),false);

	// Get the binary path
	OOBase::String strBinPath;
	if (!get_config_arg("binary_path",strBinPath))
		LOG_ERROR_RETURN(("Failed to find binary_path configuration parameter"),false);

#if defined(_WIN32)
	err = strBinPath.append("OOSvrReg.exe");
#else
	err = strBinPath.append("oosvrreg");
#endif
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

	// Write initial message
	OOBase::CDRStream stream;
	size_t mark = stream.buffer()->mark_wr_ptr();
	stream.write(OOBase::uint16_t(0));
	stream.write(OOBase::uint16_t(0));
	stream.write_string(strRegPath);

	OOBase::String strThreads;
	get_config_arg("registry_concurrency",strThreads);
	OOBase::uint8_t threads = static_cast<OOBase::uint8_t>(strtoul(strThreads.c_str(),NULL,10));
	if (threads < 1 || threads > 8)
		threads = 2;
	stream.write(threads);

	for (size_t pos = 0;pos < m_config_args.size();++pos)
	{
		if (!stream.write_string(*m_config_args.key_at(pos)) || !stream.write_string(*m_config_args.at(pos)))
			break;
	}
	stream.write("");

	stream.replace(static_cast<OOBase::uint16_t>(stream.length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	// Get our uid
	uid_t uid;
	OOBase::ScopedArrayPtr<char> strOurUName;
	if (!get_our_uid(uid,strOurUName))
		return false;

	// Spawn the process
	OOBase::Environment::env_table_t tabEnv;
	OOBase::SharedPtr<Process> ptrProcess;
	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket;
	bool bAgain;
	if (!platform_spawn(strBinPath,uid,NULL,tabEnv,ptrProcess,ptrSocket,bAgain))
		return false;

	OOBase::RefPtr<RegistryConnection> ptrRegistry = new RegistryConnection(this,ptrProcess,ptrSocket);
	if (!ptrRegistry)
		LOG_ERROR_RETURN(("Failed to create new registry connection: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	// Send the start data - blocking is OK as we have background proactor threads waiting
	err = OOBase::CDRIO::send_and_recv_with_header_blocking<OOBase::uint16_t>(stream,ptrSocket);
	if (err)
		LOG_ERROR_RETURN(("Failed to send registry start data: %s",OOBase::system_error_text(err)),false);

	OOBase::uint16_t trans_id = 0;
	OOBase::int32_t ret_err = 0;
	if (!stream.read(trans_id) || !stream.read(ret_err))
		LOG_ERROR_RETURN(("Failed to read start response from registry: %s",OOBase::system_error_text(stream.last_error())),false);
	if (ret_err)
		LOG_ERROR_RETURN(("Registry failed to start properly: %s",OOBase::system_error_text(ret_err)),false);

	// Add to registry process map
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Add to the handle map as ID 1
	err = m_registry_processes.force_insert(1,ptrRegistry);
	if (err)
		LOG_ERROR_RETURN(("Failed to insert registry handle: %s",OOBase::system_error_text(err)),false);

	guard.release();

	if (!ptrRegistry->start(1))
	{
		guard.acquire();

		m_registry_processes.remove(size_t(1));

		return false;
	}

	OOBase::Logger::log(OOBase::Logger::Information,"System registry started successfully");

	return true;
}

bool Root::Manager::spawn_user_registry(OOBase::RefPtr<ClientConnection>& ptrClient)
{
	OOBase::Logger::log(OOBase::Logger::Information,"Starting user registry...");

	OOBase::String strRegPath;
	if (!get_config_arg("regdb_path",strRegPath))
		LOG_ERROR_RETURN(("Missing 'regdb_path' config setting"),false);

	OOBase::String strUsersPath;
	get_config_arg("users_path",strUsersPath);

	OOBase::String strHive;
	if (!get_registry_hive(ptrClient->get_uid(),strRegPath,strUsersPath,strHive))
		return false;

	// Get the binary path
	OOBase::String strBinPath;
	if (!get_config_arg("binary_path",strBinPath))
		LOG_ERROR_RETURN(("Failed to find binary_path configuration parameter"),false);

#if defined(_WIN32)
	int err = strBinPath.append("OOSvrReg.exe");
#else
	int err = strBinPath.append("oosvrreg");
#endif
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

	// Spawn the process
	OOBase::Environment::env_table_t tabEnv;
	OOBase::SharedPtr<Process> ptrProcess;
	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket;
	bool bAgain;
	if (!platform_spawn(strBinPath,ptrClient->get_uid(),NULL,tabEnv,ptrProcess,ptrSocket,bAgain))
		return false;

	OOBase::RefPtr<RegistryConnection> ptrRegistry = new RegistryConnection(this,ptrProcess,ptrSocket);
	if (!ptrRegistry)
		LOG_ERROR_RETURN(("Failed to create new registry connection: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	// Add to registry process map
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Add to the registry map
	size_t handle = 0;
	err = m_registry_processes.insert(ptrRegistry,handle);
	if (err)
		LOG_ERROR_RETURN(("Failed to insert registry handle: %s",OOBase::system_error_text(err)),false);

	guard.release();

	if (!ptrRegistry->start(strHive,ptrClient,handle))
	{
		guard.acquire();

		m_registry_processes.remove(handle);

		return false;
	}

	return true;
}

OOBase::RefPtr<Root::RegistryConnection> Root::Manager::get_root_registry()
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	OOBase::RefPtr<RegistryConnection> ptr;
	OOBase::HandleTable<size_t,OOBase::RefPtr<RegistryConnection> >::iterator i = m_registry_processes.find(size_t(1));
	if (i != m_registry_processes.end())
		ptr = i->value;

	return ptr;
}

bool Root::Manager::get_registry_process(size_t id, OOBase::RefPtr<RegistryConnection>& ptrReg)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	OOBase::HandleTable<size_t,OOBase::RefPtr<RegistryConnection> >::iterator i = m_registry_processes.find(id);
	if (i == m_registry_processes.end())
		return false;

	ptrReg = i->value;
	return true;
}

void Root::Manager::drop_registry_process(size_t id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_registry_processes.remove(id);
}
