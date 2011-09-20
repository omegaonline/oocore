///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor, Jamal Natour
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
//  Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "RootManager.h"
#include "SpawnedProcess.h"

#if defined(HAVE_UNISTD_H)

#include <grp.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/wait.h>

namespace
{
	class SpawnedProcessUnix : public Root::SpawnedProcess
	{
	public:
		SpawnedProcessUnix(OOSvrBase::AsyncLocalSocket::uid_t id, bool bSandbox);
		virtual ~SpawnedProcessUnix();

		bool Spawn(int pass_fd, bool& bAgain);

		bool IsRunning() const;
		bool CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed) const;
		bool IsSameLogin(OOSvrBase::AsyncLocalSocket::uid_t uid, const char* session_id) const;
		bool IsSameUser(OOSvrBase::AsyncLocalSocket::uid_t uid) const;
		bool GetRegistryHive(OOBase::String& strSysDir, OOBase::String& strUsersDir, OOBase::LocalString& strHive);

	private:
		bool    m_bSandbox;
		uid_t   m_uid;
		pid_t   m_pid;

		void close_all_fds(int except_fd);
	};
}

SpawnedProcessUnix::SpawnedProcessUnix(OOSvrBase::AsyncLocalSocket::uid_t id, bool bSandbox) :
		m_bSandbox(bSandbox),
		m_uid(id),
		m_pid(0)
{
}

SpawnedProcessUnix::~SpawnedProcessUnix()
{
	if (m_pid != 0)
	{
		pid_t retv = 0;
		for (int i=0; i<10; ++i)
		{
			int ec;
			retv = waitpid(m_pid,&ec,WNOHANG);
			if (retv != 0)
				break;

			sleep(1);
			retv = 0;
		}

		if (retv == 0)
			kill(m_pid,SIGKILL);

		m_pid = 0;
	}
}

void SpawnedProcessUnix::close_all_fds(int except_fd)
{
	int mx = -1;

#if defined(_POSIX_OPEN_MAX) && (_POSIX_OPEN_MAX >= 0)
#if (_POSIX_OPEN_MAX == 0)
	/* value available at runtime only */
	mx = sysconf(_SC_OPEN_MAX);
#else
	/* value available at compile time */
	mx = _POSIX_OPEN_MAX;
#endif
#endif

	if (mx > 4)
	{
		for (int fd_i=STDERR_FILENO+1; fd_i<mx && fd_i != except_fd; ++fd_i)
			close(fd_i);
	}
	else
	{
		/* based on lsof style walk of proc filesystem so should
		 * work on anything with a proc filesystem i.e. a OSx/BSD */
		/* walk proc, closing all descriptors from stderr onwards for our pid */

		OOBase::LocalString str;
		int err = str.printf("/proc/%u/fd/",getpid());
		if (err != 0)
			LOG_ERROR(("Failed to format string: %s",OOBase::system_error_text(err)));
		else
		{
			DIR* pdir = NULL;
			if (!(pdir = opendir(str.c_str())))
				LOG_ERROR(("opendir failed for %s %s",str.c_str(),OOBase::system_error_text()));
			else
			{
				/* skips ./ and ../ entries in addition to skipping to the passed fd offset */
				for (dirent* pfile; (pfile = readdir(pdir));)
				{
					int fd;
					if (!('.' == *pfile->d_name || (fd = atoi(pfile->d_name))<0 || fd<STDERR_FILENO+1 || fd==except_fd))
						close(fd);
				}
			}

			closedir(pdir);
		}
	}
}

bool SpawnedProcessUnix::Spawn(int pass_fd, bool& bAgain)
{
	OOBase::LocalString strAppName;
	strAppName.getenv("OOSERVER_BINARY_PATH");
	if (strAppName.empty())
	{
		int err = strAppName.assign(LIBEXEC_DIR "/oosvruser");
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);
	}
	else
	{
		strAppName.replace('\\','/');
		int err = OOBase::AppendDirSeparator(strAppName);

		if (err == 0)
			err = strAppName.append("oosvruser");

		if (err != 0)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

		OOSvrBase::Logger::log(OOSvrBase::Logger::Warning,"Using oosvruser: %s",strAppName.c_str());
	}

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

	// Set stdin/out/err to /dev/null
	int fd = open("/dev/null",O_RDWR);
	if (fd == -1)
	{
		LOG_ERROR(("open(/dev/null) failed: %s",OOBase::system_error_text()));
		_exit(127);
	}

	// Check this session stuff with the Stevens book! umask? etc...
	void* POSIX_TODO;

	dup2(fd,STDIN_FILENO);
	dup2(fd,STDOUT_FILENO);
	dup2(fd,STDERR_FILENO);
	close(fd);

	// Close all open handles - not that we should have any ;)
	close_all_fds(pass_fd);

	// Change dir to a known location
	if (chdir(LIBEXEC_DIR) != 0)
	{
		LOG_ERROR(("chdir(%s) failed: %s",LIBEXEC_DIR,OOBase::system_error_text()));
		_exit(127);
	}

	if (bChangeUid)
	{
		// get our pw_info
		OOBase::POSIX::pw_info pw(m_uid);
		if (!pw)
		{
			LOG_ERROR(("getpwuid() failed: %s",OOBase::system_error_text()));
			_exit(127);
		}

		// Set our gid...
		if (setgid(pw->pw_gid) != 0)
		{
			LOG_ERROR(("setgid() failed: %s",OOBase::system_error_text()));
			_exit(127);
		}

		// Init our groups...
		if (initgroups(pw->pw_name,pw->pw_gid) != 0)
		{
			LOG_ERROR(("initgroups() failed: %s",OOBase::system_error_text()));
			_exit(127);
		}

		// Stop being priviledged!
		if (setuid(m_uid) != 0)
		{
			LOG_ERROR(("setuid() failed: %s",OOBase::system_error_text()));
			_exit(127);
		}
	}

	// Exec the user process
	OOBase::LocalString strPipe;
	int err = strPipe.printf("--fork-slave=%u",pass_fd);
	if (err != 0)
	{
		LOG_ERROR(("Failed to concatenate strings: %s",OOBase::system_error_text(err)));
		_exit(127);
	}

	OOBase::LocalString debug,display;
	debug.getenv("OMEGA_DEBUG");
	display.getenv("DISPLAY");
	if (debug == "yes" && !display.empty())
	{
		OOBase::LocalString strExec;
		if ((err = strExec.printf("%s %s",strAppName.c_str(),strPipe.c_str())) != 0)
		{
			LOG_ERROR(("Failed to concatenate strings: %s",OOBase::system_error_text(err)));
			_exit(127);
		}

		execlp("xterm","xterm","-T","oosvruser - Sandbox","-e",strExec.c_str(),(char*)0);
	}

	execlp(strAppName.c_str(),strAppName.c_str(),strPipe.c_str(),(char*)0);

	LOG_ERROR(("Failed to launch %s cwd: %s - %s",strAppName.c_str(),get_current_dir_name(),strerror(errno)));

	_exit(127);
}

bool SpawnedProcessUnix::IsRunning() const
{
	if (m_pid == 0)
		return false;

	pid_t retv = waitpid(m_pid,NULL,WNOHANG);
	return (retv == 0);
}

bool SpawnedProcessUnix::CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed) const
{
	bAllowed = false;

	// Get file info
	struct stat sb;
	if (stat(pszFName,&sb) != 0)
		LOG_ERROR_RETURN(("stat() failed!",OOBase::system_error_text()),false);

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
	else if (sb.st_uid == m_uid)
	{
		// Is the supplied user the file's owner
		if (mode==O_RDONLY && (sb.st_mode & S_IRUSR))
			bAllowed = true;
		else if (mode==O_WRONLY && (sb.st_mode & S_IWUSR))
			bAllowed = true;
		else if (mode==O_RDWR && (sb.st_mode & (S_IRUSR | S_IWUSR)))
			bAllowed = true;
	}
	else
	{
		void* POSIX_TODO; // Enumerate all the users groups!

		// Get the suppied user's group see if that is the same as the file's group
		OOBase::POSIX::pw_info pw(m_uid);
		if (!pw)
			LOG_ERROR_RETURN(("getpwuid() failed!",OOBase::system_error_text()),false);

		// Is the file's gid the same as the specified user's
		if (pw->pw_gid == sb.st_gid)
		{
			if (mode==O_RDONLY && (sb.st_mode & S_IRGRP))
				bAllowed = true;
			else if (mode==O_WRONLY && (sb.st_mode & S_IWGRP))
				bAllowed = true;
			else if (mode==O_RDWR && (sb.st_mode & (S_IRGRP | S_IWGRP)))
				bAllowed = true;
		}
	}

	return true;
}

bool SpawnedProcessUnix::IsSameLogin(uid_t uid, const char* session_id) const
{
	assert(!IsSameUser(uid));

	// Sort out the session handling
	void* ISSUE_5;

	// All POSIX sessions are assumed unique...
	return false;
}

bool SpawnedProcessUnix::IsSameUser(uid_t uid) const
{
	// The sandbox is a 'unique' user
	if (m_bSandbox)
		return false;

	return (m_uid == uid);
}

bool SpawnedProcessUnix::GetRegistryHive(OOBase::String& strSysDir, OOBase::String& strUsersDir, OOBase::LocalString& strHive)
{
	assert(!m_bSandbox);

	OOBase::POSIX::pw_info pw(m_uid);
	if (!pw)
		LOG_ERROR_RETURN(("getpwuid() failed: %s",OOBase::system_error_text()),false);

	int err = 0;
	if (strSysDir.empty())
	{
		if ((err = strSysDir.assign("/var/lib/omegaonline/")) != 0)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);
	}

	if ((err = OOBase::AppendDirSeparator(strSysDir)) != 0)
		LOG_ERROR_RETURN(("Failed to append separator: %s",OOBase::system_error_text(err)),false);

	if ((err = strSysDir.append("default_user.regdb")) != 0)
		LOG_ERROR_RETURN(("Failed to append strings: %s",OOBase::system_error_text(err)),false);

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

	if ((err = OOBase::AppendDirSeparator(strUsersDir)) != 0)
		LOG_ERROR_RETURN(("Failed to append separator: %s",OOBase::system_error_text(err)),false);

	if (bAddDot)
	{
		if ((err = strUsersDir.append(".omegaonline")) != 0)
			LOG_ERROR_RETURN(("Failed to assign strings: %s",OOBase::system_error_text(err)),false);
	}
	else
	{
		if ((err = strHive.append(pw->pw_name)) != 0)
			LOG_ERROR_RETURN(("Failed to assign strings: %s",OOBase::system_error_text(err)),false);
	}

	if ((err = strHive.append(".regdb")) != 0)
		LOG_ERROR_RETURN(("Failed to assign strings: %s",OOBase::system_error_text(err)),false);

	// Check hive exists... if it doesn't copy default_user.regdb and chown/chmod correctly
	int fd_to = ::open(strHive.c_str(),O_CREAT | O_EXCL | O_WRONLY | S_IRUSR | S_IWUSR);
	if (fd_to!= -1)
	{
		// If we get here, then we have a brand new file...
		int fd_from = ::open(strSysDir.c_str(),O_RDONLY);
		if (fd_from == -1)
		{
			err = errno;
			::close(fd_to);
			::unlink(strHive.c_str());
			LOG_ERROR_RETURN(("Failed to open %s: %s",strSysDir.c_str(),OOBase::system_error_text(err)),false);
		}

		char buffer[1024] = {0};
		ssize_t r = 0;
		do
		{
			do
			{
				r = read(fd_from,buffer,sizeof(buffer));
			} while (r==-1 && errno==EINTR);

			if (r == -1)
			{
				err = errno;
				::close(fd_from);
				::close(fd_to);
				::unlink(strHive.c_str());
				LOG_ERROR_RETURN(("Failed to copy file contents: %s",OOBase::system_error_text(err)),false);
			}

			if (r > 0)
			{
				ssize_t s = 0;
				do
				{
					s = write(fd_to,buffer,r);
				} while (s==-1 && errno==EINTR);

				if (s != r)
				{
					err = errno;
					::close(fd_from);
					::close(fd_to);
					::unlink(strHive.c_str());
					LOG_ERROR_RETURN(("Failed to copy file contents: %s",OOBase::system_error_text(err)),false);
				}
			}
		} while (r != 0);

		::close(fd_from);
		::close(fd_to);

		if (chown(strHive.c_str(),m_uid,pw->pw_gid) == -1)
		{
			err = errno;
			::unlink(strHive.c_str());
			LOG_ERROR_RETURN(("Failed to set file owner: %s",OOBase::system_error_text(err)),false);
		}
	}

	return true;
}

OOBase::SmartPtr<Root::SpawnedProcess> Root::Manager::platform_spawn(OOSvrBase::AsyncLocalSocket::uid_t uid, const char* session_id, OOBase::String& strPipe, Omega::uint32_t& channel_id, OOBase::RefPtr<OOServer::MessageConnection>& ptrMC, bool& bAgain)
{
	// Create a pair of sockets
	int fd[2] = {-1, -1};
	if (socketpair(PF_UNIX,SOCK_STREAM,0,fd) != 0)
		LOG_ERROR_RETURN(("socketpair() failed: %s",OOBase::system_error_text()),(SpawnedProcess*)0);

	// Add FD_CLOEXEC to fd[0]
	int err = OOBase::POSIX::set_close_on_exec(fd[0],true);
	if (err != 0)
	{
		::close(fd[0]);
		::close(fd[1]);
		LOG_ERROR_RETURN(("set_close_on_exec() failed: %s",OOBase::system_error_text(err)),(SpawnedProcess*)0);
	}

	// Alloc a new SpawnedProcess
	SpawnedProcessUnix* pSpawnUnix = new (std::nothrow) SpawnedProcessUnix(uid,session_id == NULL);
	if (!pSpawnUnix)
	{
		::close(fd[0]);
		::close(fd[1]);
		LOG_ERROR_RETURN(("Out of memory"),(SpawnedProcess*)0);
	}
	OOBase::SmartPtr<Root::SpawnedProcess> pSpawn = pSpawnUnix;

	// Spawn the process
	if (!pSpawnUnix->Spawn(fd[1],bAgain))
	{
		::close(fd[0]);
		::close(fd[1]);
		return 0;
	}

	// Done with fd[1]
	::close(fd[1]);

	// Create an async socket wrapper
	OOBase::RefPtr<OOSvrBase::AsyncLocalSocket> ptrSocket = Proactor::instance().attach_local_socket(fd[0],err);
	if (err != 0)
	{
		::close(fd[0]);
		LOG_ERROR_RETURN(("Failed to attach socket: %s",OOBase::system_error_text(err)),(SpawnedProcess*)0);
	}

	// Bootstrap the user process...
	channel_id = bootstrap_user(ptrSocket,ptrMC,strPipe);
	if (!channel_id)
		return 0;

	return pSpawn;
}

/*void Root::Manager::accept_client(OOSvrBase::AsyncLocalSocketPtr ptrSocket)
{
	// Socket will close when it drops out of scope

	OOSvrBase::AsyncLocalSocket::uid_t uid;
	int err = ptrSocket->get_uid(uid);
	if (err != 0)
		LOG_ERROR(("Failed to retrieve client token: %s",OOBase::system_error_text(err)));
	else
	{
		// Make sure we have a user process
		UserProcess user_process;
		if (get_user_process(uid,session_id,user_process))
		{
			UserProcess new_process;
			new_process.ptrRegistry = user_process.ptrRegistry;
			new_process.ptrSpawn = new (std::nothrow) SpawnedProcessUnix(uid,false);
			if (!new_process.ptrSpawn)
			{
				LOG_ERROR(("Out of memory"));
				return;
			}

			// Bootstrap the user process...
			OOBase::SmartPtr<OOServer::MessageConnection> ptrMC;
			Omega::uint32_t channel_id = bootstrap_user(ptrSocket,ptrMC,new_process.strPipe);
			if (channel_id)
			{
				// Insert the data into the process map...
				OOBase::Guard<OOBase::RWMutex> guard(m_lock);

				err = m_mapUserProcesses.insert(channel_id,new_process);
				if (err != 0)
				{
					ptrMC->close();
					LOG_ERROR(("Failed to insert into map: %s",OOBase::system_error_text(err)));
					return;
				}

				// Now start the read cycle from ptrMC
				ptrMC->read();
			}
		}
	}
}*/

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
