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
#include <sys/wait.h>

namespace
{
	class RootProcessUnix : public Root::Process
	{
	public:
		RootProcessUnix(OOSvrBase::AsyncLocalSocket::uid_t id);
		virtual ~RootProcessUnix();

		bool Spawn(OOBase::String& strAppName, const char* session_id, int pass_fd, bool& bAgain);

		bool IsRunning() const;
		int CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed) const;
		bool IsSameLogin(OOSvrBase::AsyncLocalSocket::uid_t uid, const char* session_id) const;
		bool IsSameUser(OOSvrBase::AsyncLocalSocket::uid_t uid) const;
		bool GetRegistryHive(OOBase::String strSysDir, OOBase::String strUsersDir, OOBase::LocalString& strHive);

	private:
		OOBase::String m_sid;
		bool           m_bSandbox;
		uid_t          m_uid;
		pid_t          m_pid;
	};

	void exit_msg(const char* fmt, ...)
	{
		va_list args;
		va_start(args,fmt);

		OOBase::LocalString msg;
		int err = msg.vprintf(fmt,args);

		va_end(args);

		if (err == 0)
			OOBase::stderr_write(msg.c_str());

		_exit(127);
	}
}

RootProcessUnix::RootProcessUnix(OOSvrBase::AsyncLocalSocket::uid_t id) :
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
				retv = waitpid(m_pid,&ec,WNOHANG);
				if (retv != 0)
					break;

				sleep(1);
				retv = 0;
			}
		}
		else
		{
			int ec;
			retv = waitpid(m_pid,&ec,0);
		}

		if (retv == 0)
			kill(m_pid,SIGKILL);

		m_pid = 0;
	}
}

bool RootProcessUnix::Spawn(OOBase::String& strAppName, const char* session_id, int pass_fd, bool& bAgain)
{
	m_bSandbox = (session_id == NULL);
	int err = m_sid.assign(session_id);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

	if ((err = OOBase::Paths::CorrectDirSeparators(strAppName)) == 0 &&
			(err = OOBase::Paths::AppendDirSeparator(strAppName)) == 0)
	{
		err = strAppName.append("oosvruser");
	}

	if (err != 0)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

	char* rpath = realpath(strAppName.c_str(),NULL);
	OOBase::Logger::log(OOBase::Logger::Information,"Using oosvruser: %s",rpath);
	::free(rpath);

	// Check the file exists
	if (access(strAppName.c_str(),X_OK) != 0)
		LOG_ERROR_RETURN(("User process %s is not valid: %s",strAppName.c_str(),OOBase::system_error_text()),false);

	// Check our uid
	uid_t our_uid = getuid();

	bool bChangeUid = (m_uid != our_uid);
	if (bChangeUid && our_uid != 0)
	{
		bAgain = true;
		return false;
	}

	pid_t child_id = fork();
	if (child_id == -1)
		LOG_ERROR_RETURN(("fork() failed: %s",OOBase::system_error_text()),false);

	if (child_id != 0)
	{
		// We are the parent
		m_pid = child_id;
		return true;
	}

	// We are the child...

	if (bChangeUid)
	{
		// get our pw_info
		OOBase::POSIX::pw_info pw(m_uid);
		if (!pw)
			exit_msg("getpwuid() failed: %s\n",OOBase::system_error_text());

		// Set our gid...
		if (setgid(pw->pw_gid) != 0)
			exit_msg("setgid() failed: %s\n",OOBase::system_error_text());

		// Init our groups...
		if (initgroups(pw->pw_name,pw->pw_gid) != 0)
			exit_msg("initgroups() failed: %s\n",OOBase::system_error_text());

		// Stop being privileged!
		if (setuid(m_uid) != 0)
			exit_msg("setuid() failed: %s\n",OOBase::system_error_text());
	}

	// Build the pipe name
	OOBase::LocalString strPipe;
	if ((err = strPipe.printf("--pipe=%u",pass_fd)) != 0)
		exit_msg("Failed to concatenate strings: %s\n",OOBase::system_error_text(err));

	// Close all open handles
	int except[] = { STDERR_FILENO, pass_fd };
	err = OOBase::POSIX::close_file_descriptors(except,sizeof(except)/sizeof(except[0]));
	if (err)
		exit_msg("close_file_descriptors() failed: %s\n",OOBase::system_error_text(err));

	int n = OOBase::POSIX::open("/dev/null",O_RDONLY);
	if (n == -1)
		exit_msg("Failed to open /dev/null: %s\n",OOBase::system_error_text(err));

	// Now close off stdin/stdout/stderr
	dup2(n,STDIN_FILENO);
	dup2(n,STDOUT_FILENO);
	dup2(n,STDERR_FILENO);
	OOBase::POSIX::close(n);

	OOBase::LocalString display;
	display.getenv("DISPLAY");
	if (Root::is_debug() && !display.empty())
	{
		OOBase::String strTitle;
		if (m_bSandbox)
			strTitle.assign("oosvruser:/sandbox");
		else
			strTitle.concat("oosvruser:",m_sid.c_str());

		execlp("xterm","xterm","-T",strTitle.c_str(),"-e",strAppName.c_str(),strPipe.c_str(),"--debug",(char*)NULL);

		//OOBase::LocalString params;
		//params.printf("--log-file=vallog%d.txt",getpid());
		//execlp("xterm","xterm","-T",strTitle.c_str(),"-e","libtool","--mode=execute","valgrind","--leak-check=full",params.c_str(),strAppName.c_str(),strPipe.c_str(),"--debug",(char*)NULL);

		//OOBase::LocalString gdb;
		//gdb.printf("run %s --debug",strPipe.c_str());
		//execlp("xterm","xterm","-T",strTitle.c_str(),"-e","libtool","--mode=execute","gdb",strAppName.c_str(),"-ex",gdb.c_str(),(char*)NULL);
	}

	const char* argv[] = { "oosvruser", strPipe.c_str(), NULL, NULL };
	if (Root::is_debug())
		argv[2] = "--debug";

	execv(strAppName.c_str(),(char* const*)argv);

	err = errno;
	exit_msg("Failed to launch '%s', cwd '%s': %s\n",strAppName.c_str(),get_current_dir_name(),OOBase::system_error_text(err));
	return false;
}

bool RootProcessUnix::IsRunning() const
{
	if (m_pid == 0)
		return false;

	int status = 0;
	return (waitpid(m_pid,&status,WNOHANG) == 0);
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
		OOBase::POSIX::pw_info pw(m_uid);
		if (!pw)
		{
			int err = errno;
			LOG_ERROR_RETURN(("getpwuid() failed: %s",OOBase::system_error_text(err)),err);
		}

		OOBase::SmartPtr<gid_t,OOBase::LocalAllocator> ptrGroups;
		int ngroups = 0;
		if (getgrouplist(pw->pw_name,pw->pw_gid,NULL,&ngroups) == -1)
		{
			if (!ptrGroups.allocate(ngroups * sizeof(gid_t)))
				LOG_ERROR_RETURN(("Out of memory!"),ENOMEM);

			getgrouplist(pw->pw_name,pw->pw_gid,ptrGroups,&ngroups);
		}

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

bool RootProcessUnix::GetRegistryHive(OOBase::String strSysDir, OOBase::String strUsersDir, OOBase::LocalString& strHive)
{
	assert(!m_bSandbox);

	int err = OOBase::Paths::AppendDirSeparator(strSysDir);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to append separator: %s",OOBase::system_error_text(err)),false);

	OOBase::POSIX::pw_info pw(m_uid);
	if (!pw)
		LOG_ERROR_RETURN(("getpwuid() failed: %s",OOBase::system_error_text()),false);

	bool bAddDot = false;
	if (strUsersDir.empty())
	{
		OOBase::LocalString strHome;
		strHome.getenv("HOME");
		if (!strHome.empty())
		{
			bAddDot = true;
			err = strUsersDir.assign(strHome.c_str());
		}
		else
			err = strUsersDir.concat(strSysDir.c_str(),"users/");

		if (err != 0)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);
	}

	if ((err = OOBase::Paths::AppendDirSeparator(strUsersDir)) != 0)
		LOG_ERROR_RETURN(("Failed to append separator: %s",OOBase::system_error_text(err)),false);

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

	LOG_DEBUG(("Registry hive: %s",strHive.c_str()));

	// Check hive exists... if it doesn't copy user_template.regdb and chown/chmod correctly
	OOBase::POSIX::SmartFD fd_to(OOBase::POSIX::open(strHive.c_str(),O_CREAT | O_EXCL | O_WRONLY,S_IRUSR | S_IWUSR));
	if (fd_to == -1)
	{
		if (errno != EEXIST)
			LOG_ERROR_RETURN(("Failed to open registry hive: '%s' %s",strHive.c_str(),OOBase::system_error_text(errno)),false);
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

		if (chown(strHive.c_str(),m_uid,pw->pw_gid) == -1)
		{
			err = errno;
			::unlink(strHive.c_str());
			LOG_ERROR_RETURN(("Failed to set file owner: %s",OOBase::system_error_text(err)),false);
		}
	}

	return true;
}

OOBase::SmartPtr<Root::Process> Root::Manager::platform_spawn(OOSvrBase::AsyncLocalSocket::uid_t uid, const char* session_id, OOBase::String& strPipe, Omega::uint32_t& channel_id, OOBase::RefPtr<OOServer::MessageConnection>& ptrMC, bool& bAgain)
{
	// Create a pair of sockets
	int fd[2] = {-1, -1};
	if (socketpair(PF_UNIX,SOCK_STREAM,0,fd) != 0)
		LOG_ERROR_RETURN(("socketpair() failed: %s",OOBase::system_error_text()),OOBase::SmartPtr<Root::Process>());

	// Add FD_CLOEXEC to fd[0]
	int err = OOBase::POSIX::set_close_on_exec(fd[0],true);
	if (err != 0)
	{
		OOBase::POSIX::close(fd[0]);
		OOBase::POSIX::close(fd[1]);
		LOG_ERROR_RETURN(("set_close_on_exec() failed: %s",OOBase::system_error_text(err)),OOBase::SmartPtr<Root::Process>());
	}

	// Alloc a new RootProcess
	RootProcessUnix* pSpawnUnix = new (std::nothrow) RootProcessUnix(uid);
	if (!pSpawnUnix)
	{
		OOBase::POSIX::close(fd[0]);
		OOBase::POSIX::close(fd[1]);
		LOG_ERROR_RETURN(("Out of memory"),OOBase::SmartPtr<Root::Process>());
	}

	OOBase::String strAppName;
	if ((err = strAppName.assign(LIBEXEC_DIR)) != 0)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),OOBase::SmartPtr<Root::Process>());

	// If we are debugging, allow binary_path override
	if (Root::is_debug())
	{
		if (get_config_arg("binary_path",strAppName))
			OOBase::Logger::log(OOBase::Logger::Warning,"Overriding with 'binary_path' setting '%s'",strAppName.c_str());
	}

	// Spawn the process
	if (!pSpawnUnix->Spawn(strAppName,session_id,fd[1],bAgain))
	{
		OOBase::POSIX::close(fd[0]);
		OOBase::POSIX::close(fd[1]);
		return OOBase::SmartPtr<Root::Process>();
	}

	// Done with fd[1]
	OOBase::POSIX::close(fd[1]);

	// Create an async socket wrapper
	OOBase::RefPtr<OOSvrBase::AsyncLocalSocket> ptrSocket = Proactor::instance().attach_local_socket(fd[0],err);
	if (err != 0)
	{
		OOBase::POSIX::close(fd[0]);
		LOG_ERROR_RETURN(("Failed to attach socket: %s",OOBase::system_error_text(err)),OOBase::SmartPtr<Root::Process>());
	}

	// Bootstrap the user process...
	channel_id = bootstrap_user(ptrSocket,ptrMC,strPipe);
	if (!channel_id)
		return OOBase::SmartPtr<Root::Process>();

	return OOBase::SmartPtr<Root::Process>(pSpawnUnix);
}

bool Root::Manager::get_our_uid(OOSvrBase::AsyncLocalSocket::uid_t& uid, OOBase::LocalString& strUName)
{
	uid = getuid();

	OOBase::POSIX::pw_info pw(uid);
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

bool Root::Manager::get_sandbox_uid(const OOBase::String& strUName, OOSvrBase::AsyncLocalSocket::uid_t& uid, bool& bAgain)
{
	bAgain = false;

	// Resolve to uid
	OOBase::POSIX::pw_info pw(strUName.c_str());
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
