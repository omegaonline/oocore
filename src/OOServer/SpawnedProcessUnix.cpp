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

#if defined(HAVE_UNISTD_H) && !defined(_WIN32)

#include <stdio.h>
#include <grp.h>
#include <sys/wait.h>
#include <dirent.h>

#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#if defined(HAVE_SYS_FCNTL_H)
#include <sys/fcntl.h>
#endif /* HAVE_SYS_FCNTL_H */

#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif

namespace
{
	class SpawnedProcessUnix : public Root::SpawnedProcess
	{
	public:
		SpawnedProcessUnix(OOSvrBase::AsyncLocalSocket::uid_t id, bool bSandbox);
		virtual ~SpawnedProcessUnix();

		bool Spawn(std::string strAppPath, bool bUnsafe, int pass_fd);

		bool CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed) const;
		bool Compare(OOSvrBase::AsyncLocalSocket::uid_t uid) const;
		bool IsSameUser(OOSvrBase::AsyncLocalSocket::uid_t uid) const;
		bool GetRegistryHive(const std::string& strSysDir, const std::string& strUsersDir, std::string& strHive);

	private:
		bool    m_bSandbox;
		uid_t   m_uid;
		pid_t   m_pid;

		bool clean_environment();
		void close_all_fds(int except_fd);
	};

	static bool y_or_n_p(const char* question)
	{
		fputs(question,stdout);
		while (1)
		{
			// Write a space
			fputc(' ',stdout);

			// Read first char of line
			int c = fgetc(stdin);
			int answer = c;

			// Discard rest of line
			while (c != '\n' && c != EOF)
				c = fgetc(stdin);

			if (answer == 'Y')
				return true;
			else if (answer == 'n')
				return false;

			// Invalid answer
			fputs("Please answer Y or n:",stdout);
		}
	}
}

SpawnedProcessUnix::SpawnedProcessUnix(OOSvrBase::AsyncLocalSocket::uid_t id, bool bSandbox) :
		m_bSandbox(bSandbox),
		m_uid(id),
		m_pid(-1)
{
}

SpawnedProcessUnix::~SpawnedProcessUnix()
{
	if (m_pid != pid_t(-1))
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

		m_pid = pid_t(-1);
	}
}

extern char **environ;

bool SpawnedProcessUnix::clean_environment()
{
	// Add other safe environment variable names here
	// NOTE: PATH is set by hand!
	static const char* safe_env_lst[] =
	{
		"LANG",
		"TZ",
		NULL
	};

	static const size_t env_max = 256;
	char** cleanenv = static_cast<char**>(calloc(env_max,sizeof(char*)));
	if (!cleanenv)
		return false;

	cleanenv[0] = strdup("PATH=/usr/local/bin:/usr/bin:/bin");

	size_t cidx = 1;
	for (char** ep = environ; *ep && cidx < env_max-1; ep++)
	{
		if (!strncmp(*ep,"OMEGA_",6)==0)
		{
			cleanenv[cidx] = *ep;
			cidx++;
		}
		else
		{
			for (size_t idx = 0; safe_env_lst[idx]; idx++)
			{
				if (strncmp(*ep,safe_env_lst[idx],strlen(safe_env_lst[idx]))==0)
				{
					cleanenv[cidx] = *ep;
					cidx++;
					break;
				}
			}
		}
	}

	void* POSIX_TODO; // Add USER, TERM etc...

	cleanenv[cidx] = NULL;
	environ = cleanenv;

	return true;
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

	void* POSIX_TODO; // what about getdtablesize()?

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
		DIR *pdir;
		char str[1024] = {0};

		snprintf(str,sizeof(str)-1,"/proc/%u/fd/",getpid());

		if (!(pdir = opendir(str)))
		{
			LOG_ERROR(("opendir failed for %s %s",str,OOBase::system_error_text(errno).c_str()));
			return;
		}

		/* skips ./ and ../ entries in addition to skipping to the passed fd offset */
		for (dirent* pfile; (pfile = readdir(pdir));)
		{
			int fd;
			if (!('.' == *pfile->d_name || (fd = atoi(pfile->d_name))<0 || fd<STDERR_FILENO+1 || fd==except_fd))
				close(fd);
		}

		closedir(pdir);
	}
}

bool SpawnedProcessUnix::Spawn(std::string strAppPath, bool bUnsafe, int pass_fd)
{
	if (strAppPath.empty())
		strAppPath = LIBEXEC_DIR "/oosvruser";
	else
		OOSvrBase::Logger::log(OOSvrBase::Logger::Warning,"Using user_host: %s",strAppPath.c_str());

	// Check our uid
	bool bUnsafeStart = false;
	uid_t our_uid = getuid();
	if (our_uid != 0)
	{
		if (!bUnsafe)
			LOG_ERROR_RETURN(("OOServer must be started as root."),false);

		OOSvrBase::pw_info pw(our_uid);
		if (!pw)
			LOG_ERROR_RETURN(("getpwuid() failed: %s",OOBase::system_error_text(errno).c_str()),false);

		// Prompt for continue...
		OOSvrBase::Logger::log(OOSvrBase::Logger::Warning,
							   "ooserverd is running under a user account that does not have the priviledges required to fork and setuid as a different user.\n\n"
							   "Because the 'unsafe' mode is set the new user process will be started under the user account '%s'\n\n"
							   "This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.",
							   pw->pw_name);

		bUnsafeStart = true;
	}

	pid_t child_id = fork();
	if (child_id == -1)
	{
		// Error
		LOG_ERROR_RETURN(("fork() failed: %s",OOBase::system_error_text(errno).c_str()),false);
	}
	else if (child_id == 0)
	{
		// We are the child...

		// This all needs sorting out badly!
		// We should be spawning the user's shell with -c
		// Set pwd to user's home dir, etc...
		void* POSIX_TODO;

		if (!bUnsafeStart)
		{
			// get our pw_info
			OOSvrBase::pw_info pw(m_uid);
			if (!pw)
			{
				LOG_ERROR(("getpwuid() failed: %s",OOBase::system_error_text(errno).c_str()));
				exit(errno);
			}

			// Set our gid...
			if (setgid(pw->pw_gid) != 0)
			{
				LOG_ERROR(("setgid() failed: %s",OOBase::system_error_text(errno).c_str()));
				exit(errno);
			}

			// Init our groups...
			if (initgroups(pw->pw_name,pw->pw_gid) != 0)
			{
				LOG_ERROR(("initgroups() failed: %s",OOBase::system_error_text(errno).c_str()));
				exit(errno);
			}

			// Stop being priviledged!
			if (setuid(m_uid) != 0)
			{
				LOG_ERROR(("setuid() failed: %s",OOBase::system_error_text(errno).c_str()));
				exit(errno);
			}
		}

		// Close all open handles - not that we should have any ;)
		close_all_fds(pass_fd);

		// Clean up environment...
		if (!clean_environment())
			exit(errno);

		// Exec the user process
		std::ostringstream os;
		os.imbue(std::locale::classic());
		os << "--fork-slave=" << pass_fd;

		char* cmd_line[3] = { 0,0,0 };
		cmd_line[0] = strdup(strAppPath.c_str());
		cmd_line[1] = strdup(os.str().c_str());

		int err = execv(strAppPath.c_str(),cmd_line);

		free(cmd_line[0]);
		free(cmd_line[1]);

		_exit(127);
	}
	else
	{
		// We are the parent...
		m_pid = child_id;
	}

	return true;
}

bool SpawnedProcessUnix::CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed) const
{
	bAllowed = false;

	// Get file info
	struct stat sb;
	if (stat(pszFName,&sb) != 0)
		LOG_ERROR_RETURN(("stat() failed!",OOBase::system_error_text(errno).c_str()),false);

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
		OOSvrBase::pw_info pw(m_uid);
		if (!pw)
			LOG_ERROR_RETURN(("getpwuid() failed!",OOBase::system_error_text(errno).c_str()),false);

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

bool SpawnedProcessUnix::Compare(uid_t uid) const
{
	return (m_uid == uid && m_pid != pid_t(-1));
}

bool SpawnedProcessUnix::IsSameUser(uid_t uid) const
{
	// The sandbox is a 'unique' user
	if (m_bSandbox)
		return false;

	return Compare(uid);
}

bool SpawnedProcessUnix::GetRegistryHive(const std::string& strSysDir, const std::string& strUsersDir, std::string& strHive)
{
	assert(!m_bSandbox);

	OOSvrBase::pw_info pw(m_uid);
	if (!pw)
		LOG_ERROR_RETURN(("getpwuid() failed: %s",OOBase::system_error_text(errno).c_str()),false);

	if (strUsersDir.empty())
		strHive = strSysDir + "users/";
	else
	{
		strHive = strUsersDir;

		if (*strHive.rbegin() != '/')
			strHive += '/';
	}

	strHive += pw->pw_name;
	strHive += ".regdb";

	// Check hive exists... if it doesn't copy default_user.regdb and chown/chmod correctly
	void* TODO;

	return true;
}

OOBase::SmartPtr<Root::SpawnedProcess> Root::Manager::platform_spawn(OOSvrBase::AsyncLocalSocket::uid_t uid, std::string& strPipe, Omega::uint32_t& channel_id, OOBase::SmartPtr<OOServer::MessageConnection>& ptrMC)
{
	// Stash the sandbox flag because we adjust uid...
	bool bSandbox = (uid == OOSvrBase::AsyncLocalSocket::uid_t(-1));
	if (bSandbox)
	{
		// Get username from config
		std::map<std::string,std::string>::const_iterator i=m_config_args.find("sandbox_uname");
		if (i == m_config_args.end())
			LOG_ERROR_RETURN(("Missing 'sandbox_uname' config setting"),(SpawnedProcess*)0);

		if (i->second.empty())
			uid = getuid();
		else
		{
			// Resolve to uid
			OOSvrBase::pw_info pw(i->second.c_str());
			if (!pw)
			{
				if (errno)
					LOG_ERROR_RETURN(("getpwnam(%s) failed: %s",i->second.c_str(),OOBase::system_error_text(errno).c_str()),(SpawnedProcess*)0);
				else
					LOG_ERROR_RETURN(("There is no account for the user '%s'",i->second.c_str()),(SpawnedProcess*)0);
			}

			uid = pw->pw_uid;
		}
	}

	// Create a pair of sockets
	int fd[2] = {-1, -1};
	if (socketpair(PF_UNIX,SOCK_STREAM,0,fd) != 0)
		LOG_ERROR_RETURN(("socketpair() failed: %s",OOBase::system_error_text(errno).c_str()),(SpawnedProcess*)0);

	// Add FD_CLOEXEC to fd[0]
	int err = OOBase::BSD::set_close_on_exec(fd[0],true);
	if (err != 0)
	{
		::close(fd[0]);
		::close(fd[1]);
		LOG_ERROR_RETURN(("fcntl() failed: %s",OOBase::system_error_text(err).c_str()),(SpawnedProcess*)0);
	}

	// Alloc a new SpawnedProcess
	SpawnedProcessUnix* pSpawnUnix = 0;
	OOBASE_NEW(pSpawnUnix,SpawnedProcessUnix(uid,bSandbox));
	if (!pSpawnUnix)
	{
		::close(fd[0]);
		::close(fd[1]);
		LOG_ERROR_RETURN(("Out of memory"),(SpawnedProcess*)0);
	}
	OOBase::SmartPtr<Root::SpawnedProcess> pSpawn = pSpawnUnix;

	// Spawn the process
	bool bUnsafe = (m_cmd_args.find("unsafe") != m_cmd_args.end());

	std::string strAppName;
	std::map<std::string,std::string>::const_iterator a = m_config_args.find("user_host");
	if (a != m_config_args.end())
		strAppName = a->second;

	if (!pSpawnUnix->Spawn(strAppName,bUnsafe,fd[1]))
	{
		::close(fd[0]);
		::close(fd[1]);
		return 0;
	}

	// Done with fd[1]
	::close(fd[1]);

	// Create an async socket wrapper
	OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket> ptrSocket = Proactor::instance()->attach_local_socket(fd[0],&err);
	if (err != 0)
	{
		::close(fd[0]);
		LOG_ERROR_RETURN(("Failed to attach socket: %s",OOBase::system_error_text(err).c_str()),(SpawnedProcess*)0);
	}

	// Bootstrap the user process...
	channel_id = bootstrap_user(ptrSocket,ptrMC,strPipe);
	if (!channel_id)
		return 0;

	return pSpawn;
}

void Root::Manager::accept_client(OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket>& ptrSocket)
{
	// Socket will close when it drops out of scope

	OOSvrBase::AsyncLocalSocket::uid_t uid;
	int err = ptrSocket->get_uid(uid);
	if (err != 0)
		LOG_ERROR(("Failed to retrieve client token: %s",OOBase::system_error_text(err).c_str()));
	else
	{
		// Make sure we have a user process
		UserProcess user_process;
		if (get_user_process(uid,user_process))
		{
			// Alloc a new SpawnedProcess
			SpawnedProcessUnix* pSpawnUnix = 0;
			OOBASE_NEW(pSpawnUnix,SpawnedProcessUnix(uid,false));
			if (!pSpawnUnix)
			{
				LOG_ERROR(("Out of memory"));
				return;
			}

			UserProcess new_process;
			new_process.ptrRegistry = user_process.ptrRegistry;
			new_process.ptrSpawn = pSpawnUnix;

			// Bootstrap the user process...
			OOBase::SmartPtr<OOServer::MessageConnection> ptrMC;
			Omega::uint32_t channel_id = bootstrap_user(ptrSocket,ptrMC,new_process.strPipe);
			if (channel_id)
			{
				// Insert the data into the process map...
				try
				{
					OOBase::Guard<OOBase::RWMutex> guard(m_lock);

					m_mapUserProcesses.insert(std::map<Omega::uint32_t,UserProcess>::value_type(channel_id,new_process));
				}
				catch (std::exception& e)
				{
					ptrMC->close();
					LOG_ERROR(("std::exception thrown %s",e.what()));
				}

				// Now start the read cycle from ptrMC
				ptrMC->read();
			}
		}
	}
}

#endif // !HAVE_UNISTD_H
