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
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "RootManager.h"
#include "SpawnedProcess.h"

#if defined(HAVE_UNISTD_H)

#include <stdio.h>
#include <grp.h>
#include <sys/wait.h>
#include <dirent.h>


// Will need this!
#include "posix_utils.h"
/*
	char szBuf[64];
#if defined(OMEGA_DEBUG)
    OOBase::timeval_t now = OOBase::gettimeofday();
	snprintf(szBuf,63,"%lx-%lx",uid,now.tv_usec);
#else
	snprintf(szBuf,63,"%lx",uid);
#endif

	if (mkdir("/tmp/omegaonline",S_IRWXU | S_IRWXG | S_IRWXO) != 0)
	{
		int err = last_error();
		if (err != EEXIST)
			return "";
	}

	// Try to make it public
	chmod("/tmp/omegaonline",S_IRWXU | S_IRWXG | S_IRWXO);

	// Attempt to remove anything already there
	unlink(("/tmp/omegaonline/" + strPrefix + szBuf).c_str());

	return "/tmp/omegaonline/" + strPrefix + szBuf;
*/

namespace
{
	class SpawnedProcessUnix : public Root::SpawnedProcess
	{
	public:
		SpawnedProcessUnix();
		virtual ~SpawnedProcessUnix();

		bool Spawn(const std::wstring& strAppPath, int nUnsafe, OOBase::LocalSocket::uid_t id, int pass_fd, bool bSandbox);

		bool CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed);
		bool Compare(OOBase::LocalSocket::uid_t uid);
		bool IsSameUser(OOBase::LocalSocket::uid_t uid);
		bool GetRegistryHive(const std::string& strSysDir, const std::string& strUsersDir, std::string& strHive);

	private:
		bool    m_bSandbox;
		uid_t	m_uid;
		pid_t	m_pid;

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
			int c = tolower(fgetc(stdin));
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

SpawnedProcessUnix::SpawnedProcessUnix() :
	m_pid(-1)
{
}

SpawnedProcessUnix::~SpawnedProcessUnix()
{
	if (m_pid != pid_t(-1))
	{
		pid_t retv = 0;
		for (int i=0;i<10;++i)
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
			LOG_ERROR(("opendir failed for %s %s",str,OOSvrBase::Logger::format_error(errno).c_str()));
			return;
		}

		/* skips ./ and ../ entries in addition to skipping to the passed fd offset */
		for (dirent* pfile; (pfile = readdir(pdir)); )
		{
			int fd;
			if (!('.' == *pfile->d_name || (fd = atoi(pfile->d_name))<0 || fd<STDERR_FILENO+1 || fd==except_fd))
				close(fd);
		}

		closedir(pdir);
	}
}

bool SpawnedProcessUnix::Spawn(const std::wstring& strAppPath, int nUnsafe, uid_t uid, int pass_fd, bool bSandbox)
{
	OOSvrBase::Logger::log(OOSvrBase::Logger::Warning,"Using user_host: %ls",strAppPath.c_str());

	m_bSandbox = bSandbox;

	// Check our uid
	bool bUnsafeStart = false;
	uid_t our_uid = getuid();
	if (our_uid != 0)
	{
		if (!nUnsafe)
			LOG_ERROR_RETURN(("OOServer must be started as root."),false);

		OOSvrBase::pw_info pw(our_uid);
		if (!pw)
			LOG_ERROR_RETURN(("getpwuid() failed: %s",OOSvrBase::Logger::format_error(errno).c_str()),false);

		// Prompt for continue...
		OOSvrBase::Logger::log(OOSvrBase::Logger::Warning,
			"ooserverd is running under a user account that does not have the priviledges required to fork and setuid as a different user.\n\n"
			"Because the 'unsafe' mode is set the new user process will be started under the user account '%s'\n\n"
			"This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.",
			pw->pw_name);

		if (nUnsafe != 2)
		{
			if (!y_or_n_p("\n\nDo you want to allow this? [y/n]:"))
				return false;

			OOSvrBase::Logger::log(OOSvrBase::Logger::Warning,"You chose to continue... on your head be it!");
		}

		bUnsafeStart = true;
	}

	pid_t child_id = fork();
	if (child_id == -1)
	{
		// Error
		LOG_ERROR_RETURN(("fork() failed: %s",OOSvrBase::Logger::format_error(errno).c_str()),false);
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
			OOSvrBase::pw_info pw(uid);
			if (!pw)
			{
				LOG_ERROR(("getpwuid() failed: %s",OOSvrBase::Logger::format_error(errno).c_str()));
				exit(errno);
			}

			// Set our gid...
			if (setgid(pw->pw_gid) != 0)
			{
				LOG_ERROR(("setgid() failed: %s",OOSvrBase::Logger::format_error(errno).c_str()));
				exit(errno);
			}

			// Init our groups...
			if (initgroups(pw->pw_name,pw->pw_gid) != 0)
			{
				LOG_ERROR(("initgroups() failed: %s",OOSvrBase::Logger::format_error(errno).c_str()));
				exit(errno);
			}

			// Stop being priviledged!
			if (setuid(uid) != 0)
			{
				LOG_ERROR(("setuid() failed: %s",OOSvrBase::Logger::format_error(errno).c_str()));
				exit(errno);
			}
		}

		// Close all open handles - not that we should have any ;)
		close_all_fds(pass_fd);

        // Clean up environment...
		if (!clean_environment())
			exit(errno);

        // Exec the user process
		const char* cmd_line[] =
		{
		    "./oosvruser", //  argv[0] = Process name
		    0,             //  argv[1] = Pipe name
		    0
		};
		std::ostringstream os;
		os << pass_fd;

		cmd_line[1] = os.str().c_str();

		int err = execv("./oosvruser",(char**)cmd_line);

		LOG_DEBUG(("Child process exiting with code: %d",err));

		exit(err);
	}
	else
	{
		// We are the parent...
		m_uid = uid;
		m_pid = child_id;

		LOG_DEBUG(("Starting new oosvruser process as uid:%u pid:%u",uid,child_id));
	}

	return true;
}

bool SpawnedProcessUnix::CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed)
{
	bAllowed = false;

	// Get file info
	struct stat sb;
	if (stat(pszFName,&sb) != 0)
		LOG_ERROR_RETURN(("stat() failed!",OOSvrBase::Logger::format_error(errno).c_str()),false);

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
			LOG_ERROR_RETURN(("getpwuid() failed!",OOSvrBase::Logger::format_error(errno).c_str()),false);

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

bool SpawnedProcessUnix::Compare(uid_t uid)
{
	return (m_uid == uid);
}

bool SpawnedProcessUnix::IsSameUser(uid_t uid)
{
	// The sandbox is a 'unique' user
	if (m_bSandbox)
		return false;

	return Compare(uid);
}

bool SpawnedProcessUnix::GetRegistryHive(const std::string& strSysDir, const std::string& strUsersDir, std::string& strHive)
{
	std::string strDir;
	if (m_bSandbox)
		strDir = "/var/lib/omegaonline";
	else
	{
		OOSvrBase::pw_info pw(m_uid);
		if (!pw)
			LOG_ERROR_RETURN(("getpwuid() failed: %s",OOSvrBase::Logger::format_error(errno).c_str()),false);

		strDir = pw->pw_dir;
		strDir += "/.omegaonline";
	}

	if(!create_unless_existing_directory(strDir,S_IRWXU | S_IRGRP ))
		LOG_ERROR_RETURN(("create_unless_existing_directory(%s) failed: %s",strDir.c_str(),OOSvrBase::Logger::format_error(errno).c_str()),false);

	strHive = strDir + "/user.regdb";

	// Check hive exists... if it doesn't copy default_user.regdb and chown/chmod correctly
	void* TODO;

	return true;
}

OOBase::SmartPtr<Root::SpawnedProcess> Root::Manager::platform_spawn(OOBase::LocalSocket::uid_t uid, std::string& strPipe, Omega::uint32_t& channel_id, OOBase::SmartPtr<MessageConnection>& ptrMC)
{
	// Stash the sandbox flag because we adjust uid...
	bool bSandbox = (uid == OOBase::LocalSocket::uid_t(-1));
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
					LOG_ERROR_RETURN(("getpwnam(%s) failed: %s",i->second.c_str(),OOSvrBase::Logger::format_error(errno).c_str()),(SpawnedProcess*)0);
				else
					LOG_ERROR_RETURN(("There is no account for the user '%s'",i->second.c_str()),(SpawnedProcess*)0);
			}

			uid = pw->pw_uid;
		}
	}

	// Create a pair of sockets
	int fd[2] = {-1, -1};
	if (socketpair(PF_UNIX,SOCK_STREAM,0,fd) != 0)
		LOG_ERROR_RETURN(("socketpair() failed: %s",OOSvrBase::Logger::format_error(errno).c_str()),(SpawnedProcess*)0);

	// Wrap fd[0]
	OOBase::POSIX::LocalSocket sock(fd[0]);

	// Add FD_CLOEXEC to fd[0]
	int oldflags = fcntl(fd[0],F_GETFD);
	if (oldflags == -1 ||
		fcntl(fd[0],F_SETFD,oldflags | FD_CLOEXEC) == -1)
	{
		int err = errno;
		::close(fd[1]);
		LOG_ERROR_RETURN(("fcntl() failed: %s",OOSvrBase::Logger::format_error(err).c_str()),(SpawnedProcess*)0);
	}

	// Alloc a new SpawnedProcess
	SpawnedProcessUnix* pSpawnUnix = 0;
	OOBASE_NEW(pSpawnUnix,SpawnedProcessUnix);
	if (!pSpawnUnix)
	{
		::close(fd[1]);
		LOG_ERROR_RETURN(("Out of memory"),(SpawnedProcess*)0);
	}
	OOBase::SmartPtr<Root::SpawnedProcess> pSpawn = pSpawnUnix;

	// Spawn the process
	int nUnsafe = 0;
	if (m_cmd_args.find("unsafe") != m_cmd_args.end())
	{
		if (m_cmd_args.find("batch") != m_cmd_args.end())
			nUnsafe = 2;
		else
			nUnsafe = 1;
	}

	std::wstring strAppName;
	std::map<std::string,std::string>::const_iterator a = m_config_args.find("user_host");
	if (a != m_config_args.end())
		strAppName = OOBase::from_utf8(a->second.c_str());

	if (!pSpawnUnix->Spawn(strAppName,nUnsafe,uid,fd[1],bSandbox))
	{
		::close(fd[1]);
		return 0;
	}

	// Done with fd[1]
	::close(fd[1]);

	// Bootstrap the user process...
	channel_id = bootstrap_user(&sock,ptrMC,strPipe);
	if (!channel_id)
		return 0;

	// Create an async socket wrapper
	int err = 0;
	OOSvrBase::AsyncSocket* pAsync = Proactor::instance()->attach_socket(ptrMC,&err,&sock);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to attach socket: %s",OOSvrBase::Logger::format_error(err).c_str()),(SpawnedProcess*)0);

	// Attach the async socket to the message connection
	ptrMC->attach(pAsync);

	return pSpawn;
}

#endif // !HAVE_UNISTD_H
