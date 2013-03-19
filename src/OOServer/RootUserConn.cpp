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

Root::UserConnection::UserConnection(Manager* pManager, OOBase::SmartPtr<Process>& ptrProcess, OOBase::RefPtr<OOBase::AsyncSocket>& ptrSocket) :
		m_pManager(pManager),
		m_ptrProcess(ptrProcess),
		m_ptrSocket(ptrSocket)
{
}

bool Root::UserConnection::same_login(const uid_t& uid, const char* session_id) const
{
	return m_ptrProcess->same_login(uid,session_id);
}

const uid_t& Root::UserConnection::get_uid() const
{
	return m_ptrProcess->get_uid();
}

pid_t Root::UserConnection::get_pid() const
{
	return m_ptrProcess->get_pid();
}

Root::UserConnection::AutoDrop::~AutoDrop()
{
	if (m_id)
		m_pManager->drop_user_process(m_id);
}

bool Root::UserConnection::start()
{
	OOBase::RefPtr<OOBase::Buffer> buffer = OOBase::Buffer::create(256,OOBase::CDRStream::MaxAlignment);
	if (!buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	addref();

	int err = m_ptrSocket->recv(this,&UserConnection::on_response,buffer);
	if (err)
	{
		release();

		LOG_ERROR_RETURN(("Failed to start response io: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

void Root::UserConnection::on_response(OOBase::Buffer* buffer, int err)
{
	if (err)
		LOG_ERROR(("Failed to receive from user process: %s",OOBase::system_error_text(err)));
	else if (buffer->length() == 0)
	{
		LOG_WARNING(("User process %lu has closed",(unsigned long)m_ptrProcess->get_pid()));
		err = -1;
	}

	while (!err && buffer->length() >= sizeof(Omega::uint16_t))
	{
		OOBase::CDRStream response(buffer);

		Omega::uint16_t len = 0;
		if (!response.read(len))
		{
			err = response.last_error();
			LOG_ERROR(("Failed to receive from user process: %s",OOBase::system_error_text(err)));
		}
		else if (buffer->length() < len - sizeof(Omega::uint16_t))
		{
			// Read the rest of the message
			err = m_ptrSocket->recv(this,&UserConnection::on_response,buffer,len);
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
		err = m_ptrSocket->recv(this,&UserConnection::on_response,buffer);
		if (err)
			LOG_ERROR(("Failed to continue response io: %s",OOBase::system_error_text(err)));
	}

	if (err)
	{
		m_pManager->drop_user_process(m_ptrProcess->get_pid());

		release();
	}
}

#if defined(_WIN32)

bool Root::UserConnection::start(pid_t client_id, const OOBase::String& fd_root)
{
	OOBase::AsyncResponseDispatcher<Omega::uint16_t>::AutoDrop response_id(m_async_dispatcher);
	int err = m_async_dispatcher.add_response(this,&UserConnection::on_started,client_id,response_id);
	if (err)
		LOG_ERROR_RETURN(("Failed to add async response: %s",OOBase::system_error_text(err)),false);

	OOBase::String str1,str2;
	return do_start(fd_root,str1,str2,response_id);
}

bool Root::UserConnection::start(pid_t client_id, const OOBase::String& fd_user, const OOBase::String& fd_root)
{
	OOBase::AsyncResponseDispatcher<Omega::uint16_t>::AutoDrop response_id(m_async_dispatcher);
	int err = m_async_dispatcher.add_response(this,&UserConnection::on_started,client_id,response_id);
	if (err)
		LOG_ERROR_RETURN(("Failed to add async response: %s",OOBase::system_error_text(err)),false);

}

bool Root::UserConnection::do_start(const OOBase::String& fd_user, const OOBase::String& fd_root, const OOBase::String& fd_sandbox, OOBase::AsyncResponseDispatcher<Omega::uint16_t>::AutoDrop& response_id)
{
	if (!start())
		return false;

	OOBase::CDRStream stream;
	size_t mark = stream.buffer()->mark_wr_ptr();
	stream.write(Omega::uint16_t(0));
	response_id.write(stream);

	// Non-Omega_Initialize payload here
	stream.write_string(fd_user);
	stream.write_string(fd_root);
	stream.write_string(fd_sandbox);

	// Align everything, ready for the Omega_Initialize call
	stream.buffer()->align_wr_ptr(OOBase::CDRStream::MaxAlignment);

	// Other stuff here!

	stream.replace(static_cast<Omega::uint16_t>(stream.length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	addref();

	int err = m_ptrSocket->send(this,&UserConnection::on_sent,stream.buffer());
	if (err)
	{
		release();

		LOG_ERROR_RETURN(("Failed to send user process data: %s",OOBase::system_error_text(err)),false);
	}

	response_id.detach();
	return true;
}

bool Root::UserConnection::add_client(pid_t client_id)
{
	//return ptrClient->send_response(fds[1],m_ptrProcess->get_pid());
	return false;
}

bool Root::UserConnection::new_connection(pid_t client_id, OOBase::AsyncResponseDispatcher<Omega::uint16_t>::AutoDrop& response_id)
{
	OOBase::RefPtr<ClientConnection> ptrClient;
	if (!m_pManager->get_client(client_id,ptrClient))
		return true;

	OOBase::StackAllocator<256> allocator;
	OOBase::TempPtr<void> ptrSIDLogon(allocator);
	DWORD dwErr = OOBase::Win32::GetLogonSID(ptrClient->get_uid(),ptrSIDLogon);
	if (dwErr)
		LOG_ERROR_RETURN(("Failed to get logon SID: %s",OOBase::system_error_text(dwErr)),false);

	OOBase::LocalString strSID(allocator);
	dwErr = OOBase::Win32::SIDToString(static_cast<void*>(ptrSIDLogon),strSID);
	if (dwErr)
		LOG_ERROR_RETURN(("Failed to format logon SID: %s",OOBase::system_error_text(dwErr)),false);

	OOBase::CDRStream stream;
	size_t mark = stream.buffer()->mark_wr_ptr();
	stream.write(Omega::uint16_t(0));

	stream.write(static_cast<OOServer::Root2User_OpCode_t>(OOServer::Root2User_NewConnection));
	response_id.write(stream);
	stream.write(client_id);
	stream.write_string(strSID);

	stream.replace(static_cast<Omega::uint16_t>(stream.length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	addref();

	int err = m_ptrSocket->send(this,&UserConnection::on_sent,stream.buffer());
	if (err)
	{
		m_pManager->drop_user_process(m_ptrProcess->get_pid());

		release();

		LOG_ERROR_RETURN(("Failed to send user process data: %s",OOBase::system_error_text(err)),false);
	}

	response_id.detach();

	return true;
}

#elif defined(HAVE_UNISTD_H)
bool Root::UserConnection::start(pid_t client_id, OOBase::POSIX::SmartFD& fd_root)
{
	OOBase::AsyncResponseDispatcher<Omega::uint16_t>::AutoDrop response_id(m_async_dispatcher);
	int err = m_async_dispatcher.add_response(this,&UserConnection::on_started,client_id,response_id);
	if (err)
		LOG_ERROR_RETURN(("Failed to add async response: %s",OOBase::system_error_text(err)),false);

	OOBase::POSIX::SmartFD fd1,fd2;
	return do_start(fd_root,fd1,fd2,response_id);
}

bool Root::UserConnection::start(pid_t client_id, OOBase::POSIX::SmartFD& fd_user, OOBase::POSIX::SmartFD& fd_root)
{
	OOBase::AsyncResponseDispatcher<Omega::uint16_t>::AutoDrop response_id(m_async_dispatcher);
	int err = m_async_dispatcher.add_response(this,&UserConnection::on_started,client_id,response_id);
	if (err)
		LOG_ERROR_RETURN(("Failed to add async response: %s",OOBase::system_error_text(err)),false);

	OOBase::POSIX::SmartFD fd1;
	return do_start(fd_user,fd_root,fd1,response_id);
}

bool Root::UserConnection::do_start(OOBase::POSIX::SmartFD& fd_user, OOBase::POSIX::SmartFD& fd_root, OOBase::POSIX::SmartFD& fd_sandbox, OOBase::AsyncResponseDispatcher<Omega::uint16_t>::AutoDrop& response_id)
{
	if (!start())
		return false;

	OOBase::RefPtr<OOBase::Buffer> ctl_buffer = OOBase::Buffer::create(CMSG_SPACE(sizeof(int)*3),sizeof(size_t));
	if (!ctl_buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	struct msghdr msg = {0};
	msg.msg_control = ctl_buffer->wr_ptr();
	msg.msg_controllen = ctl_buffer->space();

	struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;

	if (fd_sandbox.is_valid())
	{
		cmsg->cmsg_len = CMSG_LEN(sizeof(int) * 3);
		int* fds = (int*)CMSG_DATA(cmsg);
		fds[0] = fd_user;
		fds[1] = fd_root;
		fds[2] = fd_sandbox;
	}
	else if (fd_root.is_valid())
	{
		cmsg->cmsg_len = CMSG_LEN(sizeof(int) * 2);
		int* fds = (int*)CMSG_DATA(cmsg);
		fds[0] = fd_user;
		fds[1] = fd_root;
	}
	else
	{
		cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		*(int*)CMSG_DATA(cmsg) = fd_user;
	}
	ctl_buffer->wr_ptr(cmsg->cmsg_len);

	OOBase::CDRStream stream;
	size_t mark = stream.buffer()->mark_wr_ptr();
	stream.write(Omega::uint16_t(0));
	response_id.write(stream);

	// Non-Omega_Initialize payload here

	// Align everything, ready for the Omega_Initialize call
	stream.buffer()->align_wr_ptr(OOBase::CDRStream::MaxAlignment);

	// Other stuff here!

	stream.replace(static_cast<Omega::uint16_t>(stream.length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	addref();

	int err = m_ptrSocket->send_msg(this,&UserConnection::on_sent_msg,stream.buffer(),ctl_buffer);
	if (err)
	{
		release();

		LOG_ERROR_RETURN(("Failed to send user process data: %s",OOBase::system_error_text(err)),false);
	}

	fd_user.detach();
	fd_root.detach();
	fd_sandbox.detach();
	response_id.detach();

	return true;
}

bool Root::UserConnection::add_client(pid_t client_id)
{
	// Create a pair of sockets
	OOBase::POSIX::SmartFD fds[2];
	int err = OOBase::POSIX::socketpair(SOCK_STREAM,fds);
	if (err)
		LOG_ERROR_RETURN(("socketpair() failed: %s",OOBase::system_error_text(err)),false);

	OOBase::AsyncResponseDispatcher<Omega::uint16_t>::AutoDrop response_id(m_async_dispatcher);
	err = m_async_dispatcher.add_response(this,&UserConnection::on_add_client,client_id,static_cast<int>(fds[0]),response_id);
	if (err)
		LOG_ERROR_RETURN(("Failed to enqueue response: %s",OOBase::system_error_text(err)),false);

	if (!new_connection(client_id,fds[1],response_id))
		return false;

	fds[0].detach();
	return true;
}

bool Root::UserConnection::new_connection(pid_t client_id, OOBase::POSIX::SmartFD& fd, OOBase::AsyncResponseDispatcher<Omega::uint16_t>::AutoDrop& response_id)
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
	stream.write(Omega::uint16_t(0));

	stream.write(static_cast<OOServer::Root2User_OpCode_t>(OOServer::Root2User_NewConnection));
	response_id.write(stream);
	stream.write(client_id);

	stream.replace(static_cast<Omega::uint16_t>(stream.length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	addref();

	int err = m_ptrSocket->send_msg(this,&UserConnection::on_sent_msg,stream.buffer(),ctl_buffer);
	if (err)
	{
		m_pManager->drop_user_process(m_ptrProcess->get_pid());

		release();

		LOG_ERROR_RETURN(("Failed to send user process data: %s",OOBase::system_error_text(err)),false);
	}

	fd.detach();
	response_id.detach();

	return true;
}

bool Root::UserConnection::on_add_client(OOBase::CDRStream& response, pid_t client_id, int client_fd)
{
	OOBase::POSIX::SmartFD ptrClientFd(client_fd);

	OOBase::RefPtr<ClientConnection> ptrClient;
	if (!m_pManager->get_client(client_id,ptrClient))
		return true;

	ClientConnection::AutoDrop drop(m_pManager,client_id);

	Omega::int32_t ret_err = 0;
	if (!response.read(ret_err))
		LOG_ERROR_RETURN(("Failed to read add client response: %s",OOBase::system_error_text(response.last_error())),true);
	if (ret_err)
		LOG_WARNING_RETURN(("User process refused user connection: %s",OOBase::system_error_text(ret_err)),true);

	if (ptrClient->send_response(ptrClientFd,m_ptrProcess->get_pid()))
		drop.detach();

	return true;
}

void Root::UserConnection::on_sent_msg(OOBase::Buffer* data, OOBase::Buffer* ctl_buffer, int err)
{
	// Make sure we close all file handles...
	ctl_buffer->mark_rd_ptr(0);

	struct msghdr msgh = {0};
	msgh.msg_control = const_cast<char*>(ctl_buffer->rd_ptr());
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

bool Root::UserConnection::on_started(OOBase::CDRStream& stream, pid_t client_id)
{
	bool bSuccess = false;

	ClientConnection::AutoDrop drop(m_pManager,client_id);

	Omega::int32_t ret_err = 0;
	if (!stream.read(ret_err))
		LOG_ERROR(("Failed to read start response from user process: %s",OOBase::system_error_text(stream.last_error())));
	else if (ret_err)
		LOG_ERROR(("User process failed to start properly: %s",OOBase::system_error_text(ret_err)));
	else
	{
		bSuccess = true;

		if (client_id && add_client(client_id))
			drop.detach();

		OOBase::Logger::log(OOBase::Logger::Information,"User process started successfully");
	}

	if (!bSuccess)
		m_pManager->drop_user_process(m_ptrProcess->get_pid());

	return bSuccess;
}

void Root::UserConnection::on_sent(OOBase::Buffer* buffer, int err)
{
	if (err)
	{
		LOG_ERROR(("Failed to send data to user process: %s",OOBase::system_error_text(err)));

		buffer->mark_rd_ptr(0);

		OOBase::CDRStream stream(buffer);

		Omega::uint16_t len = 0;
		Omega::uint16_t trans_id = 0;
		if (stream.read(len) && stream.read(trans_id) && trans_id)
			m_async_dispatcher.drop_response(trans_id);

		m_pManager->drop_user_process(m_ptrProcess->get_pid());
	}

	release();
}

bool Root::Manager::spawn_sandbox_process(OOBase::AllocatorInstance& allocator)
{
	OOBase::Logger::log(OOBase::Logger::Information,"Starting system sandbox...");

	OOBase::LocalString strUnsafe(allocator);
	bool bUnsafe = false;
	if (Root::is_debug() && get_config_arg("unsafe",strUnsafe))
		bUnsafe = (strUnsafe == "true");

	// Get username from config
	OOBase::LocalString strUName(allocator);
	if (!get_config_arg("sandbox_uname",strUName) && !bUnsafe)
		LOG_ERROR_RETURN(("Failed to find the 'sandbox_uname' setting in the config"),false);

	bool bAgain = false;
	uid_t uid;
	if (strUName.empty())
	{
		OOBase::LocalString strOurUName(allocator);
		if (!get_our_uid(uid,strOurUName))
			return false;

		// Warn!
		OOBase::Logger::log(OOBase::Logger::Warning,
			"Because the 'unsafe' mode is set the sandbox process will be started under the current user account '%s'.\n\n"
			"This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.\n",
			strOurUName.c_str());
	}
	else if (!get_sandbox_uid(strUName,uid,bAgain))
	{
		if (bAgain && bUnsafe)
		{
			OOBase::LocalString strOurUName(allocator);
			if (!get_our_uid(uid,strOurUName))
				return false;

			OOBase::Logger::log(OOBase::Logger::Warning,
								   APPNAME " is running under a user account that does not have the privileges required to impersonate a different user.\n\n"
								   "Because the 'unsafe' mode is set the sandbox process will be started under the current user account '%s'.\n\n"
								   "This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.\n",
								   strOurUName.c_str());
		}
		else
			return false;
	}

	// Get the environment settings
	OOBase::Environment::env_table_t tabSysEnv(allocator);
#if defined(_WIN32)
	int err = OOBase::Environment::get_user(uid,tabSysEnv);
#else
	int err = OOBase::Environment::get_current(tabSysEnv);
#endif
	if (err)
		LOG_ERROR_RETURN(("Failed to load environment variables: %s",OOBase::system_error_text(err)),false);

	// Get sandbox environment from root registry
	OOBase::RefPtr<RegistryConnection> ptrRoot = get_root_registry();

	OOBase::Environment::env_table_t tabEnv(allocator);
	if (!ptrRoot->get_environment("/System/Sandbox/Environment",tabEnv))
		return false;

	err = OOBase::Environment::substitute(tabEnv,tabSysEnv);
	if (err)
		LOG_ERROR_RETURN(("Failed to substitute environment variables: %s",OOBase::system_error_text(err)),false);

	// Get the binary path
	OOBase::LocalString strBinPath(allocator);
	if (!get_config_arg("binary_path",strBinPath))
		LOG_ERROR_RETURN(("Failed to find binary_path configuration parameter"),false);

#if defined(_WIN32)
	err = strBinPath.append("OOSvrUser.exe");
#else
	err = strBinPath.append("oosvruser");
#endif
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

	// Spawn the process
	OOBase::SmartPtr<Process> ptrProcess;
	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket;
	bool res = platform_spawn(strBinPath,uid,NULL,tabSysEnv,ptrProcess,ptrSocket,bAgain);
	if (!res && bAgain && bUnsafe && !strUName.empty())
	{
		OOBase::LocalString strOurUName(allocator);
		if (!get_our_uid(uid,strOurUName))
			return false;

		OOBase::Logger::log(OOBase::Logger::Warning,
							   APPNAME " is running under a user account that does not have the privileges required to create new processes as a different user.\n\n"
							   "Because the 'unsafe' mode is set the sandbox process will be started under the current user account '%s'.\n\n"
							   "This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.\n",
							   strOurUName.c_str());


		res = platform_spawn(strBinPath,uid,NULL,tabSysEnv,ptrProcess,ptrSocket,bAgain);
	}
	if (!res)
		return false;

	m_sandbox_process = new (std::nothrow) UserConnection(this,ptrProcess,ptrSocket);
	if (!m_sandbox_process)
		LOG_ERROR_RETURN(("Failed to create new user connection: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	// Add to process map
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Add to the map
	err = m_user_processes.insert(ptrProcess->get_pid(),m_sandbox_process);
	if (err)
		LOG_ERROR_RETURN(("Failed to insert process handle: %s",OOBase::system_error_text(err)),false);

	guard.release();

	if (!ptrRoot->start_user(m_sandbox_process))
	{
		guard.acquire();

		m_user_processes.remove(ptrProcess->get_pid());
		m_sandbox_process = NULL;

		return false;
	}

	OOBase::Logger::log(OOBase::Logger::Information,"System sandbox started successfully");

	return true;
}

bool Root::Manager::spawn_user_process(pid_t client_id, OOBase::RefPtr<RegistryConnection>& ptrRegistry)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	OOBase::RefPtr<ClientConnection> ptrClient;
	if (!m_clients.find(client_id,ptrClient))
	{
		// No client?  No worries...
		return true;
	}

	guard.release();

	if (!spawn_user_process(ptrClient,ptrRegistry))
	{
		guard.acquire();

		m_clients.remove(client_id);

		return false;
	}

	return true;
}

bool Root::Manager::spawn_user_process(OOBase::RefPtr<ClientConnection>& ptrClient, OOBase::RefPtr<RegistryConnection>& ptrRegistry)
{
	OOBase::Logger::log(OOBase::Logger::Information,"Starting user process...");

	// Get the environment settings
	OOBase::StackAllocator<1024> allocator;
	OOBase::Environment::env_table_t tabSysEnv(allocator);
#if defined(_WIN32)
	int err = OOBase::Environment::get_user(ptrClient->get_uid(),tabSysEnv);
#else
	int err = OOBase::Environment::get_current(tabSysEnv);
#endif
	if (err)
		LOG_ERROR_RETURN(("Failed to load environment variables: %s",OOBase::system_error_text(err)),false);

	OOBase::Environment::env_table_t tabEnv(allocator);
	if (!ptrRegistry->get_environment("/Environment",tabEnv))
		return false;

	err = OOBase::Environment::substitute(tabEnv,tabSysEnv);
	if (err)
		LOG_ERROR_RETURN(("Failed to substitute environment variables: %s",OOBase::system_error_text(err)),false);

	// Get the binary path
	OOBase::LocalString strBinPath(allocator);
	if (!get_config_arg("binary_path",strBinPath))
		LOG_ERROR_RETURN(("Failed to find binary_path configuration parameter"),false);

#if defined(_WIN32)
	err = strBinPath.append("OOSvrUser.exe");
#else
	err = strBinPath.append("oosvruser");
#endif
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

	// Spawn the process
	bool bAgain = false;
	OOBase::SmartPtr<Process> ptrProcess;
	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket;
	if (!platform_spawn(strBinPath,ptrClient->get_uid(),ptrClient->get_session_id(),tabSysEnv,ptrProcess,ptrSocket,bAgain))
		return false;

	OOBase::RefPtr<UserConnection> ptrUser = new (std::nothrow) UserConnection(this,ptrProcess,ptrSocket);
	if (!ptrUser)
		LOG_ERROR_RETURN(("Failed to create new user connection: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	// Add to process map
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Add to the map
	err = m_user_processes.insert(ptrProcess->get_pid(),ptrUser);
	if (err)
		LOG_ERROR_RETURN(("Failed to insert process handle: %s",OOBase::system_error_text(err)),false);

	guard.release();

	if (!ptrRegistry->start_user(ptrClient,ptrUser))
	{
		guard.acquire();

		m_user_processes.remove(ptrProcess->get_pid());

		return false;
	}

	return true;
}

bool Root::Manager::get_user_process(pid_t user_id, OOBase::RefPtr<UserConnection>& ptrUser)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	return m_user_processes.find(user_id,ptrUser);
}

void Root::Manager::drop_user_process(pid_t id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_user_processes.remove(id);
}

OOBase::RefPtr<Root::UserConnection> Root::Manager::get_sandbox_process()
{
	return m_sandbox_process;
}
