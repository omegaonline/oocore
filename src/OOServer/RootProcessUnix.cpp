///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
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

/////////////////////////////////////////////////////////////
//
//  ***** THIS IS A SECURE MODULE *****
//
//  It will be run as Administrator/setuid root
//
//  Therefore it needs to be SAFE AS HOUSES!
//
//  Do not include anything unnecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "RootManager.h"
#include "RootProcess.h"

#if defined(HAVE_UNISTD_H)

#include <grp.h>
#include <fcntl.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/un.h>

namespace
{
	class RootProcessUnix : public Root::Process
	{
	public:
		static RootProcessUnix* Spawn(OOBase::LocalString& strAppName, uid_t uid, const char* session_id, int pass_fd, bool& bAgain, char* const envp[]);
		virtual ~RootProcessUnix();

		int CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed) const;
		bool IsSameLogin(OOBase::AsyncLocalSocket::uid_t uid, const char* session_id) const;
		bool IsSameUser(OOBase::AsyncLocalSocket::uid_t uid) const;

		bool IsRunning() const;
		OOServer::RootErrCode LaunchService(Root::Manager* pManager, const OOBase::LocalString& strName, const Omega::int64_t& key, unsigned long wait_secs, bool async, OOBase::RefPtr<OOBase::Socket>& ptrSocket) const;

	private:
		RootProcessUnix(OOBase::AsyncLocalSocket::uid_t id);

		OOBase::String m_sid;
		bool           m_bSandbox;
		uid_t          m_uid;
		pid_t          m_pid;
	};

	void exit_msg(OOBase::AllocatorInstance& allocator, const char* fmt, ...)
	{
		va_list args;
		va_start(args,fmt);

		OOBase::TempPtr<char> msg(allocator);
		int err = OOBase::vprintf(msg,fmt,args);

		va_end(args);

		if (err == 0)
			OOBase::stderr_write(msg);

		_exit(127);
	}

	int unique_pipename(sockaddr_un& addr)
	{
		addr.sun_family = AF_UNIX;
#if defined(__linux__)
		addr.sun_path[0] = '\0';
		strncpy(addr.sun_path+1,"/org/omegaonline/",sizeof(addr.sun_path)-1);
		size_t offset = strlen(addr.sun_path+1) + 1;
#elif defined(P_tmpdir)
		strncpy(addr.sun_path,P_tmpdir "/",sizeof(addr.sun_path)-1);
		size_t offset = strlen(addr.sun_path);
#else
		strncpy(addr.sun_path,"/tmp/",sizeof(addr.sun_path)-1);
		size_t offset = strlen(addr.sun_path);
#endif

		return OOBase::POSIX::random_chars(addr.sun_path+offset,24);
	}
}

RootProcessUnix::RootProcessUnix(OOBase::AsyncLocalSocket::uid_t id) :
		m_bSandbox(true),
		m_uid(id),
		m_pid(0)
{
}

RootProcessUnix::~RootProcessUnix()
{
	if (m_pid != 0)
	{
		pid_t retv = 0;

		if (!Root::is_debug())
		{
			for (int i=0; i<5; ++i)
			{
				int ec;
				retv = OOBase::POSIX::waitpid(m_pid,&ec,WNOHANG);
				if (retv != 0)
				{
					if (retv == -1)
						LOG_ERROR(("waitpid() failed: %s",OOBase::system_error_text()));
					break;
				}

				sleep(1);
				retv = 0;
			}
		}
		else
		{
			int ec;
			retv = OOBase::POSIX::waitpid(m_pid,&ec,0);
			if (retv == -1)
				LOG_ERROR(("waitpid() failed: %s",OOBase::system_error_text()));
		}

		if (retv == 0)
			kill(m_pid,SIGKILL);

		m_pid = 0;
	}
}

RootProcessUnix* RootProcessUnix::Spawn(OOBase::LocalString& strAppName, uid_t uid, const char* session_id, int pass_fd, bool& bAgain, char* const envp[])
{
	char* rpath = realpath(strAppName.c_str(),NULL);
	if (rpath)
	{
		if (strAppName != rpath)
			OOBase::Logger::log(OOBase::Logger::Information,"Mapping %s to %s",strAppName.c_str(),rpath);
		::free(rpath);
	}

	// Check the file exists
	if (access(strAppName.c_str(),X_OK) != 0)
		LOG_ERROR_RETURN(("User process %s is not valid: %s",strAppName.c_str(),OOBase::system_error_text()),(RootProcessUnix*)NULL);

	// Check our uid
	uid_t our_uid = getuid();

	bool bChangeUid = (uid != our_uid);
	if (bChangeUid && our_uid != 0)
	{
		bAgain = true;
		return NULL;
	}

	pid_t child_id = fork();
	if (child_id == -1)
		LOG_ERROR_RETURN(("fork() failed: %s",OOBase::system_error_text()),(RootProcessUnix*)NULL);

	if (child_id != 0)
	{
		// We are the parent
		RootProcessUnix* pSpawn = new (std::nothrow) RootProcessUnix(uid);
		if (!pSpawn)
			LOG_ERROR_RETURN(("Failed to allocate: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),pSpawn);

		pSpawn->m_bSandbox = (session_id == NULL);
		int err = pSpawn->m_sid.assign(session_id);
		if (err != 0)
		{
			delete pSpawn;
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),(RootProcessUnix*)NULL);
		}

		pSpawn->m_pid = child_id;
		return pSpawn;
	}

	// We are the child...

	if (bChangeUid)
	{
		// get our pw_info
		OOBase::POSIX::pw_info pw(strAppName.get_allocator(),uid);
		if (!pw)
			exit_msg(strAppName.get_allocator(),"getpwuid() failed: %s\n",OOBase::system_error_text());

		// Set our gid...
		if (setgid(pw->pw_gid) != 0)
			exit_msg(strAppName.get_allocator(),"setgid() failed: %s\n",OOBase::system_error_text());

		// Init our groups...
		if (initgroups(pw->pw_name,pw->pw_gid) != 0)
			exit_msg(strAppName.get_allocator(),"initgroups() failed: %s\n",OOBase::system_error_text());

		// Stop being privileged!
		if (setuid(uid) != 0)
			exit_msg(strAppName.get_allocator(),"setuid() failed: %s\n",OOBase::system_error_text());
	}

	// Build the pipe name
	OOBase::LocalString strPipe(strAppName.get_allocator());
	int err = strPipe.printf("--pipe=%u",pass_fd);
	if (err)
		exit_msg(strAppName.get_allocator(),"Failed to concatenate strings: %s\n",OOBase::system_error_text(err));

	// Close all open handles
	int except[] = { STDERR_FILENO, pass_fd };
	err = OOBase::POSIX::close_file_descriptors(except,sizeof(except)/sizeof(except[0]));
	if (err)
		exit_msg(strAppName.get_allocator(),"close_file_descriptors() failed: %s\n",OOBase::system_error_text(err));

	int n = OOBase::POSIX::open("/dev/null",O_RDONLY);
	if (n == -1)
		exit_msg(strAppName.get_allocator(),"Failed to open /dev/null: %s\n",OOBase::system_error_text(err));

	// Now close off stdin/stdout/stderr
	dup2(n,STDIN_FILENO);
	dup2(n,STDOUT_FILENO);
	dup2(n,STDERR_FILENO);
	OOBase::POSIX::close(n);

	if (Root::is_debug())
	{
		OOBase::LocalString display(strAppName.get_allocator());
		OOBase::Environment::getenv("DISPLAY",display);
		if (!display.empty())
		{
			OOBase::LocalString strTitle(strAppName.get_allocator());
			if (session_id == NULL)
				strTitle.printf("%s:/sandbox",strAppName.c_str());
			else
				strTitle.printf("%s:%s",strAppName.c_str(),session_id);

			const char* argv[] = { "xterm","-T",strTitle.c_str(),"-e",strAppName.c_str(),strPipe.c_str(),"--debug",NULL };

			//OOBase::LocalString valgrind;
			//valgrind.printf("--log-file=valgrind_log%d.txt",getpid());
			//const char* argv[] = { "xterm","-T",strTitle.c_str(),"-e","libtool","--mode=execute","valgrind","--leak-check=full",valgrind.c_str(),strAppName.c_str(),strPipe.c_str(),"--debug",NULL };

			//OOBase::LocalString gdb;
			//gdb.printf("run %s --debug",strPipe.c_str());
			//const char* argv[] = { "xterm","-T",strTitle.c_str(),"-e","libtool","--mode=execute","gdb",strAppName.c_str(),"-ex",gdb.c_str(), NULL };

			execvpe(argv[0],(char* const*)argv,envp);
		}
	}

	const char* argv[] = { strAppName.c_str(), strPipe.c_str(), NULL, NULL };
	if (Root::is_debug())
		argv[2] = "--debug";

	execve(strAppName.c_str(),(char* const*)argv,envp);

	err = errno;
	exit_msg(strAppName.get_allocator(),"Failed to launch '%s', cwd '%s': %s\n",strAppName.c_str(),get_current_dir_name(),OOBase::system_error_text(err));
	return NULL;
}

bool RootProcessUnix::IsRunning() const
{
	if (m_pid == 0)
		return false;

	int status = 0;
	pid_t ret = OOBase::POSIX::waitpid(m_pid,&status,WNOHANG);
	if (ret == -1)
		LOG_ERROR(("waitpid() failed: %s",OOBase::system_error_text()));

	return (ret == 0);
}

int RootProcessUnix::CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed) const
{
	bAllowed = false;

	// Get file info
	struct stat sb;
	if (stat(pszFName,&sb) != 0)
	{
		int err = errno;
		LOG_ERROR_RETURN(("stat() failed: %s",OOBase::system_error_text(err)),err);
	}

	int mode = -1;
	if (bRead && !bWrite)
		mode = O_RDONLY;
	else if (!bRead && bWrite)
		mode = O_WRONLY;
	else if (bRead && bWrite)
		mode = O_RDWR;

	if (mode==O_RDONLY && (sb.st_mode & S_IROTH))
		bAllowed = true;
	else if (mode==O_WRONLY && (sb.st_mode & S_IWOTH))
		bAllowed = true;
	else if (mode==O_RDWR && (sb.st_mode & (S_IROTH | S_IWOTH)))
		bAllowed = true;

	if (!bAllowed && sb.st_uid == m_uid)
	{
		// Is the supplied user the file's owner
		if (mode==O_RDONLY && (sb.st_mode & S_IRUSR))
			bAllowed = true;
		else if (mode==O_WRONLY && (sb.st_mode & S_IWUSR))
			bAllowed = true;
		else if (mode==O_RDWR && (sb.st_mode & (S_IRUSR | S_IWUSR)))
			bAllowed = true;
	}

	if (!bAllowed)
	{
		// Get the supplied user's group see if that is the same as the file's group
		OOBase::StackAllocator<512> allocator;
		OOBase::POSIX::pw_info pw(allocator,m_uid);
		if (!pw)
		{
			int err = errno;
			LOG_ERROR_RETURN(("getpwuid() failed: %s",OOBase::system_error_text(err)),err);
		}

		OOBase::TempPtr<gid_t> ptrGroups(allocator);
		int ngroups = 8;
		do
		{
			if (!ptrGroups.reallocate(ngroups))
				LOG_ERROR_RETURN(("Failed to allocate groups: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),ERROR_OUTOFMEMORY);
		}
		while (getgrouplist(pw->pw_name,pw->pw_gid,ptrGroups,&ngroups) == -1);

		for (int i = 0; i< ngroups && !bAllowed; ++i)
		{
			// Is the file's gid the same as the specified user's
			if (ptrGroups[i] == sb.st_gid)
			{
				if (mode==O_RDONLY && (sb.st_mode & S_IRGRP))
					bAllowed = true;
				else if (mode==O_WRONLY && (sb.st_mode & S_IWGRP))
					bAllowed = true;
				else if (mode==O_RDWR && (sb.st_mode & (S_IRGRP | S_IWGRP)))
					bAllowed = true;
			}
		}
	}

	return 0;
}

bool RootProcessUnix::IsSameLogin(uid_t uid, const char* session_id) const
{
	if (!IsSameUser(uid))
		return false;

	// Compare session_ids
	return (m_sid == session_id);
}

bool RootProcessUnix::IsSameUser(uid_t uid) const
{
	// The sandbox is a 'unique' user
	if (m_bSandbox)
		return false;

	return (m_uid == uid);
}

OOServer::RootErrCode RootProcessUnix::LaunchService(Root::Manager* pManager, const OOBase::LocalString& strName, const Omega::int64_t& key, unsigned long wait_secs, bool async, OOBase::RefPtr<OOBase::Socket>& ptrSocket) const
{
	// Create a new socket
	OOBase::POSIX::SmartFD fd(::socket(AF_UNIX,SOCK_STREAM,0));
	if (!fd.is_valid())
		LOG_ERROR_RETURN(("Failed to create socket: %s",OOBase::system_error_text()),OOServer::Errored);

	// Set permissions
	if (::fchmod(fd,0666) != 0)
		LOG_ERROR_RETURN(("Failed to set socket permissions: %s",OOBase::system_error_text()),OOServer::Errored);

	// Bind to a unique pipe name
	int err = 0;
	sockaddr_un addr = {0};
	for (;;)
	{
		err = unique_pipename(addr);
		if (err)
			LOG_ERROR_RETURN(("Failed to generate unique pipe name: %s",OOBase::system_error_text(err)),OOServer::Errored);

		socklen_t addr_len = 0;
		if (addr.sun_path[0] == '\0')
			addr_len = offsetof(sockaddr_un, sun_path) + strlen(addr.sun_path+1) + 1;
		else
			addr_len = offsetof(sockaddr_un, sun_path) + strlen(addr.sun_path);

		err = OOBase::Net::bind(fd,(const sockaddr*)&addr,addr_len);
		if (!err)
			break;

		if (err != EADDRINUSE)
			LOG_ERROR_RETURN(("Failed to bind pipe: %s",OOBase::system_error_text(err)),OOServer::Errored);
	}

	// Invent a random secret...
	char secret[32] = {0};
	err = OOBase::POSIX::random_chars(secret,sizeof(secret));
	if (err)
		LOG_ERROR_RETURN(("Failed to read random bytes: %s",OOBase::system_error_text(err)),OOServer::Errored);

	OOBase::LocalString strPipe(strName.get_allocator());
	if (addr.sun_path[0] == '\0')
		err = strPipe.concat(" ",addr.sun_path+1);
	else
		err = strPipe.append(addr.sun_path);
	if (err)
		LOG_ERROR_RETURN(("Failed to append string: %s",OOBase::system_error_text(err)),OOServer::Errored);

	// Send the pipe name and the rest of the service info to the sandbox oosvruser process
	OOBase::CDRStream request;
	if (!request.write(static_cast<OOServer::RootOpCode_t>(OOServer::Service_Start)) ||
			!request.write_string(strPipe) ||
			!request.write_string(strName) ||
			!request.write(key) ||
			!request.write(secret))
	{
		LOG_ERROR_RETURN(("Failed to write request data: %s",OOBase::system_error_text(request.last_error())),OOServer::Errored);
	}

	// Set the socket listening
	if (::listen(fd,1) != 0)
		LOG_ERROR_RETURN(("Failed to listen on pipe: %s",OOBase::system_error_text()),OOServer::Errored);

	OOBase::CDRStream response;
	OOServer::MessageHandler::io_result::type res = pManager->sendrecv_sandbox(request,async ? NULL : &response,static_cast<Omega::uint16_t>(async ? OOServer::Message_t::asynchronous : OOServer::Message_t::synchronous));
	if (res != OOServer::MessageHandler::io_result::success)
		LOG_ERROR_RETURN(("Failed to send service request to sandbox"),OOServer::Errored);

	if (!async)
	{
		OOServer::RootErrCode_t err2;
		if (!response.read(err2))
			LOG_ERROR_RETURN(("Failed to read response: %s",OOBase::system_error_text(response.last_error())),OOServer::Errored);

		if (err2)
		{
			OOBase::Logger::log(OOBase::Logger::Error,"Failed to start service '%s'.  Check error log for details.",strName.c_str());
			return static_cast<OOServer::RootErrCode>(err2);
		}
	}

	// accept() a connection
	OOBase::POSIX::SmartFD new_fd;
	OOBase::Timeout timeout(wait_secs,0);
	if (Root::is_debug())
		timeout = OOBase::Timeout();

	err = OOBase::Net::accept(fd,new_fd,timeout);
	if (err == ETIMEDOUT)
	{
		OOBase::Logger::log(OOBase::Logger::Error,"Timed out waiting for service '%s' to start",strName.c_str());
		return OOServer::Errored;
	}
	else if (err)
		LOG_ERROR_RETURN(("Failed to accept: %s",OOBase::system_error_text(err)),OOServer::Errored);

	ptrSocket = OOBase::Socket::attach_local(new_fd.detach(),err);
	if (err)
		LOG_ERROR_RETURN(("Failed to attach socket: %s",OOBase::system_error_text(err)),OOServer::Errored);

	// Now read the secret back...
	char secret2[32] = {0};
	ptrSocket->recv(secret2,sizeof(secret2)-1,true,err,timeout);
	if (err)
		LOG_ERROR_RETURN(("Failed to read from socket: %s",OOBase::system_error_text(err)),OOServer::Errored);

	uid_t other_uid;
	err = ptrSocket->get_peer_uid(other_uid);
	if (err)
		LOG_ERROR_RETURN(("Failed to determine service user: %s",OOBase::system_error_text(err)),OOServer::Errored);

	// Check the secret and the uid
	if (memcmp(secret,secret2,sizeof(secret)) == 0 && other_uid == m_uid)
		return OOServer::Ok;

	OOBase::Logger::log(OOBase::Logger::Error,"Failed to validate service");
	return OOServer::Errored;
}

bool Root::Manager::get_registry_hive(OOBase::AsyncLocalSocket::uid_t uid, OOBase::LocalString strSysDir, OOBase::LocalString strUsersDir, OOBase::LocalString& strHive)
{
	int err = 0;
	OOBase::POSIX::pw_info pw(strSysDir.get_allocator(),uid);
	if (!pw)
		LOG_ERROR_RETURN(("getpwuid() failed: %s",OOBase::system_error_text()),false);

	bool bAddDot = false;
	if (strUsersDir.empty())
	{
		OOBase::LocalString strHome(strSysDir.get_allocator());
		err = strHome.assign(pw->pw_dir);
		if (err)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

		if (strHome.empty())
			OOBase::Environment::getenv("HOME",strHome);

		if (!strHome.empty())
		{
			bAddDot = true;
			err = strUsersDir.assign(strHome.c_str());
			if (!err && strHome[strHome.length()-1] != '/')
				err = strHome.append("/",1);
		}
		else
			err = strUsersDir.concat(strSysDir.c_str(),"users/");

		if (err)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);
	}

	if (bAddDot)
	{
		if ((err = strHive.concat(strUsersDir.c_str(),".omegaonline.regdb")) != 0)
			LOG_ERROR_RETURN(("Failed to append strings: %s",OOBase::system_error_text(err)),false);
	}
	else
	{
		if ((err = strHive.printf("%s%s.regdb",strUsersDir.c_str(),pw->pw_name)) != 0)
			LOG_ERROR_RETURN(("Failed to format strings: %s",OOBase::system_error_text(err)),false);
	}

	OOBase::Logger::log(OOBase::Logger::Information,"Loading registry hive: %s",strHive.c_str());

	// Check hive exists... if it doesn't copy user_template.regdb and chown/chmod correctly
	OOBase::POSIX::SmartFD fd_to(OOBase::POSIX::open(strHive.c_str(),O_CREAT | O_EXCL | O_WRONLY,S_IRUSR | S_IWUSR));
	if (fd_to == -1)
	{
		if (errno != EEXIST)
			LOG_ERROR_RETURN(("Failed to open registry hive: '%s' %s",strHive.c_str(),OOBase::system_error_text()),false);
	}
	else
	{
		// If we get here, then we have a brand new file...
		if ((err = strSysDir.append("user_template.regdb")) != 0)
			LOG_ERROR_RETURN(("Failed to append strings: %s",OOBase::system_error_text(err)),false);

		OOBase::POSIX::SmartFD fd_from(OOBase::POSIX::open(strSysDir.c_str(),O_RDONLY));
		if (fd_from == -1)
		{
			err = errno;
			::unlink(strHive.c_str());
			LOG_ERROR_RETURN(("Failed to open %s: %s",strSysDir.c_str(),OOBase::system_error_text(err)),false);
		}

		char buffer[1024] = {0};
		ssize_t r = 0;
		do
		{
			r = OOBase::POSIX::read(fd_from,buffer,sizeof(buffer));
			if (r == -1)
			{
				err = errno;
				::unlink(strHive.c_str());
				LOG_ERROR_RETURN(("Failed to copy file contents: %s",OOBase::system_error_text(err)),false);
			}

			if (r > 0)
			{
				ssize_t s = OOBase::POSIX::write(fd_to,buffer,r);
				if (s != r)
				{
					if (s != -1)
						err = EIO;
					else
						err = errno;

					::unlink(strHive.c_str());
					LOG_ERROR_RETURN(("Failed to copy file contents: %s",OOBase::system_error_text(err)),false);
				}
			}
		} while (r != 0);

		if (chown(strHive.c_str(),uid,pw->pw_gid) == -1)
		{
			err = errno;
			::unlink(strHive.c_str());
			LOG_ERROR_RETURN(("Failed to set file owner: %s",OOBase::system_error_text(err)),false);
		}
	}

	return true;
}

bool Root::Manager::platform_spawn(OOBase::LocalString strAppName, OOBase::AsyncLocalSocket::uid_t uid, const char* session_id, const OOBase::Environment::env_table_t& tabEnv, OOBase::SmartPtr<Root::Process>& ptrSpawn, OOBase::RefPtr<OOBase::AsyncLocalSocket>& ptrSocket, bool& bAgain)
{
	OOBase::TempPtr<char*> ptrEnv(strAppName.get_allocator());
	int err = OOBase::Environment::get_envp(tabEnv,ptrEnv);
	if (err)
		LOG_ERROR_RETURN(("Failed to get environment block: %s",OOBase::system_error_text(err)),false);

	// Create a pair of sockets
	int fd[2] = {-1, -1};
	if (socketpair(PF_UNIX,SOCK_STREAM,0,fd) != 0)
		LOG_ERROR_RETURN(("socketpair() failed: %s",OOBase::system_error_text()),false);

	// Make sure sockets are closed
	OOBase::POSIX::SmartFD fds[2];
	fds[0] = fd[0];
	fds[1] = fd[1];

	OOBase::Logger::log(OOBase::Logger::Information,"Starting user process '%s'",strAppName.c_str());

	// Spawn the process
	ptrSpawn = RootProcessUnix::Spawn(strAppName,uid,session_id,fd[1],bAgain,ptrEnv);
	if (!ptrSpawn)
		return false;

	// Done with fd[1]
	fds[1].close();

	// Create an async socket wrapper
	ptrSocket = m_proactor->attach_local_socket(fd[0],err);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to attach socket: %s",OOBase::system_error_text(err)),false);

	fds[0].detach();

	return true;
}

bool Root::Manager::get_our_uid(OOBase::AsyncLocalSocket::uid_t& uid, OOBase::LocalString& strUName)
{
	uid = getuid();

	OOBase::POSIX::pw_info pw(strUName.get_allocator(),uid);
	if (!pw)
	{
		if (errno)
			LOG_ERROR_RETURN(("getpwuid() failed: %s",OOBase::system_error_text()),false);
		else
			LOG_ERROR_RETURN(("There is no account for uid %d",uid),false);
	}

	int err = strUName.assign(pw->pw_name);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text()),false);

	return true;
}

bool Root::Manager::get_sandbox_uid(const OOBase::LocalString& strUName, OOBase::AsyncLocalSocket::uid_t& uid, bool& bAgain)
{
	bAgain = false;

	// Resolve to uid
	OOBase::POSIX::pw_info pw(strUName.get_allocator(),strUName.c_str());
	if (!pw)
	{
		if (errno)
			LOG_ERROR_RETURN(("getpwnam(%s) failed: %s",strUName.c_str(),OOBase::system_error_text()),false);
		else
			LOG_ERROR_RETURN(("There is no account for the user '%s'",strUName.c_str()),false);
	}

	uid = pw->pw_uid;
	return true;
}

#endif // !HAVE_UNISTD_H
