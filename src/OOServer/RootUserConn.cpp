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

#if defined(_WIN32)

bool Root::UserConnection::add_client(OOBase::RefPtr<ClientConnection>& ptrClient)
{
	//return ptrClient->send_response(fds[1],m_ptrProcess->get_pid());
	return false;
}

#elif defined(HAVE_UNISTD_H)
bool Root::UserConnection::start(OOBase::POSIX::SmartFD& fd_user, OOBase::POSIX::SmartFD& fd_root)
{
	OOBase::RefPtr<OOBase::Buffer> ctl_buffer = OOBase::Buffer::create(CMSG_SPACE(sizeof(int)*2),sizeof(size_t));
	if (!ctl_buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	struct msghdr msg = {0};
	msg.msg_control = ctl_buffer->wr_ptr();
	msg.msg_controllen = ctl_buffer->space();

	struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;

	if (fd_root.is_valid())
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

	// Non-Omega_Initialize payload here

	// Align everything, ready for the Omega_Initialize call
	stream.buffer()->align_wr_ptr(OOBase::CDRStream::MaxAlignment);

	// Other stuff here!

	stream.replace(static_cast<Omega::uint16_t>(stream.buffer()->length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	addref();

	int err = m_ptrSocket->send_msg(this,&UserConnection::on_sent_msg,stream.buffer(),ctl_buffer);
	if (err)
	{
		release();

		LOG_ERROR_RETURN(("Failed to send user process data: %s",OOBase::system_error_text(err)),false);
	}
	else
	{
		fd_user.detach();
		fd_root.detach();
	}

	// All sent, we are done here!
	return true;
}

bool Root::UserConnection::add_client(OOBase::RefPtr<ClientConnection>& ptrClient)
{
	// Create a pair of sockets
	OOBase::POSIX::SmartFD fds[2];
	int err = OOBase::POSIX::socketpair(SOCK_STREAM,fds);
	if (err)
		LOG_ERROR_RETURN(("socketpair() failed: %s",OOBase::system_error_text(err)),false);

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
	*(int*)CMSG_DATA(cmsg) = fds[0];
	ctl_buffer->wr_ptr(cmsg->cmsg_len);

	OOBase::CDRStream stream;
	size_t mark = stream.buffer()->mark_wr_ptr();
	stream.write(Omega::uint16_t(0));

	stream.write(static_cast<OOServer::Root2User_OpCode_t>(OOServer::Root2User_NewConnection));
	stream.write(ptrClient->get_pid());

	stream.replace(static_cast<Omega::uint16_t>(stream.buffer()->length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	addref();

	err = m_ptrSocket->send_msg(this,&UserConnection::on_sent_msg,stream.buffer(),ctl_buffer);
	if (err)
	{
		m_pManager->drop_user_process(m_ptrProcess->get_pid());

		release();

		LOG_ERROR_RETURN(("Failed to send user process data: %s",OOBase::system_error_text(err)),false);
	}
	else
		fds[0].detach();

	return ptrClient->send_response(fds[1],m_ptrProcess->get_pid());
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

	if (err)
	{
		LOG_ERROR(("Failed to sent data to user process: %s",OOBase::system_error_text(err)));

		m_pManager->drop_user_process(m_ptrProcess->get_pid());
	}

	release();
}
#endif

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

	if (!ptrRoot->start_user(ptrUser))
	{
		guard.acquire();

		m_user_processes.remove(ptrProcess->get_pid());

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

	if (!ptrRegistry->start_user(ptrUser))
	{
		guard.acquire();

		m_user_processes.remove(ptrProcess->get_pid());

		return false;
	}

	OOBase::Logger::log(OOBase::Logger::Information,"User process started successfully");

	return ptrUser->add_client(ptrClient);
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
