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

		bool Spawn(OOBase::LocalSocket::uid_t id, const std::string& strPipe, bool bSandbox);

		bool CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed);
		bool Compare(OOBase::LocalSocket::uid_t uid);
		bool IsSameUser(OOBase::LocalSocket::uid_t uid);
		std::string GetRegistryHive();

	private:
		bool    m_bSandbox;
		uid_t	m_uid;
		pid_t	m_pid;

		bool clean_environment();
		void close_all_fds();
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

			if (answer == 'y')
				return true;
			else if (answer == 'n')
				return false;

			// Invalid answer
			fputs("Please answer y or n:",stdout);
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

void SpawnedProcessUnix::close_all_fds()
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
		for (int fd_i=STDERR_FILENO+1; fd_i<mx; ++fd_i)
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
			if (!('.' == *pfile->d_name || (((fd = atoi(pfile->d_name)))<0 || fd<STDERR_FILENO+1) ))
				close(fd);
		}

		closedir(pdir);
	}
}

bool SpawnedProcessUnix::Spawn(uid_t uid, const std::string& strPipe, bool bSandbox)
{
	m_bSandbox = bSandbox;

	// Check our uid
	bool bUnsafeStart = false;
	uid_t our_uid = getuid();
	if (our_uid != 0)
	{
#if !defined(OMEGA_DEBUG)
		if (!Manager::unsafe_sandbox())
			LOG_ERROR_RETURN(("OOServer must be started as root."),false);
		else
#endif
		if (our_uid == uid)
			bUnsafeStart = true;
		else
		{
			OOSvrBase::pw_info pw(our_uid);
			if (!pw)
				LOG_ERROR_RETURN(("getpwuid() failed: %s",OOSvrBase::Logger::format_error(errno).c_str()),false);

			const char msg[] =
				"ooserverd is running under a user account that does not have the priviledges required to fork and setuid as a different user.\n\n"
				"Because the 'Unsafe' value is set in the registry, the new user process will be started under the user account '%s'\n\n"
				"This is a security risk, and should only be allowed for debugging purposes, and only then if you really know what you are doing.";

			char szBuf[1024];
			snprintf(szBuf,1024,msg,pw->pw_name);

			// Prompt for continue...
			LOG_WARNING((szBuf));

			if (!y_or_n_p("\n\nDo you want to allow this? [y/n]:"))
				return false;

			printf("\nYou chose to continue... on your head be it!\n");

			bUnsafeStart = true;

		}
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

		// get our pw_info
		OOSvrBase::pw_info pw(uid);
		if (!pw)
		{
			LOG_ERROR(("getpwuid() failed: %s",OOSvrBase::Logger::format_error(errno).c_str()));
			exit(errno);
		}

		if (!bUnsafeStart)
		{
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
		close_all_fds();

        // Clean up environment...
		if (!clean_environment())
			exit(errno);

        // This all needs sorting out badly!
        // We should be spawning the user's shell with -c
        // Set pwd to user's home dir, etc...
        void* POSIX_TODO;

		// Exec the user process
		char* cmd_line[] =
		{
		    "./oosvruser", //  argv[0] = Process name
		    0,             //  argv[1] = Pipe name
		    0
		};
		cmd_line[1] = const_cast<char*>(strPipe.c_str());

		int err = execv("./oosvruser",cmd_line);

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
	return Compare(uid);
}

std::string SpawnedProcessUnix::GetRegistryHive()
{
	std::string strDir;
	if (m_bSandbox)
		strDir = "/var/lib/omegaonline";
	else
	{
		OOSvrBase::pw_info pw(m_uid);
		if (!pw)
			LOG_ERROR_RETURN(("getpwuid() failed: %s",OOSvrBase::Logger::format_error(errno).c_str()),"");

		strDir = pw->pw_dir;
		strDir += "/.omegaonline";
	}

	// Have a go at creating it... we catch failure elsewhere
	mkdir(strDir.c_str(),S_IRWXU | S_IRGRP);

	return strDir + "/user.regdb";
}

OOBase::SmartPtr<Root::SpawnedProcess> Root::Manager::platform_spawn(OOBase::LocalSocket::uid_t uid, std::string& strPipe, Omega::uint32_t& channel_id, OOBase::SmartPtr<MessageConnection>& ptrMC)
{
	// Stash the sandbox flag because we adjust uid...
	bool bSandbox = (uid == OOBase::LocalSocket::uid_t(-1));
	if (bSandbox)
	{
		// Get the user name and pwd...
		Omega::int64_t key = 0;
		int err = m_registry->open_key(0,key,"System\\Server\\Sandbox",0);
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to open sandbox registry key: %s",OOSvrBase::Logger::format_error(err).c_str()),(SpawnedProcess*)0);

		Omega::int64_t sb_uid = -1;
		err = m_registry->get_integer_value(key,"Uid",0,sb_uid);
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to read sandbox username from registry: %s",OOSvrBase::Logger::format_error(err).c_str()),(SpawnedProcess*)0);

		uid = static_cast<uid_t>(sb_uid);
	}

	// Work out if we are running in unsafe mode
	Omega::int64_t key = 0;
	Omega::int64_t v = 0;
	if (bSandbox && m_registry->open_key(0,key,"System\\Server\\Sandbox",0) == 0)
		m_registry->get_integer_value(key,"Unsafe",0,v);

	bool bUnsafe = (v == 1);

	// Alloc a new SpawnedProcess
	SpawnedProcessUnix* pSpawnUnix = 0;
	OOBASE_NEW(pSpawnUnix,SpawnedProcessUnix);
	if (!pSpawnUnix)
		return 0;

	// Create the named pipe
	/*OOBase::Win32::SmartHandle hPipe(CreatePipe(uid,strRootPipe));
	if (!hPipe.is_valid())
	{
		delete pSpawnUnix;
		return 0;
	}*/


	std::string strRootPipe;
	int sock_h = -1;

	// Spawn the process
	if (!pSpawnUnix->Spawn(uid,strRootPipe,bSandbox))
	{
		delete pSpawnUnix;
		return 0;
	}

	OOBase::SmartPtr<Root::SpawnedProcess> pSpawn = pSpawnUnix;

	// Wait for the connect attempt
	/*if (!WaitForConnect(hPipe))
		return 0;*/

	// Bootstrap the user process...
	//OOBase::LocalSocket sock;
	channel_id = bootstrap_user(NULL,ptrMC,strPipe);
	if (!channel_id)
		return 0;

	// Create an async socket wrapper
	int err = 0;
	OOSvrBase::AsyncSocket* pAsync = Proactor::instance()->attach_socket(ptrMC.value(),&err,NULL);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to attach socket: %s",OOSvrBase::Logger::format_error(err).c_str()),(SpawnedProcess*)0);

	// Attach the async socket to the message connection
	ptrMC->attach(pAsync);

	return pSpawn;
}

#endif // !HAVE_UNISTD_H
