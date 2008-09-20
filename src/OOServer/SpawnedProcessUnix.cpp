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

#include "./OOServer_Root.h"

#if !defined(ACE_WIN32)

#include "./SpawnedProcess.h"

#include <grp.h>

bool IsDebuggerPresent()
{
	void* TICKET_95;	// Do something clever here with ptrace()?
	return false;
}

#ifdef OMEGA_DEBUG
void AttachDebugger(pid_t pid)
{
	kill(pid,SIGTRAP);
}
#endif

// Helper for the recusive getpwuid_r fns()
namespace Root
{
	class pw_info
	{
	public:
		pw_info(uid_t uid);
		~pw_info();

		inline struct passwd* operator ->()
		{
			return m_pwd;
		}

		inline bool operator !() const
		{
			return (m_pwd==0);
		}

	private:
		pw_info() {};

		struct passwd* m_pwd;
		struct passwd  m_pwd2;
		char*          m_pBuffer;
		size_t         m_buf_len;
	};
}

Root::pw_info::pw_info(uid_t uid) :
m_pwd(0), m_pBuffer(0), m_buf_len(1024)
{
#ifdef _SC_GETPW_R_SIZE_MAX
	m_buf_len = sysconf(_SC_GETPW_R_SIZE_MAX) + 1;
#endif

	// _SC_GETPW_R_SIZE_MAX is defined on Mac OS X. However,
	// sysconf(_SC_GETPW_R_SIZE_MAX) returns an error. Therefore, the
	// constant is used as below when error was retured.
	if (m_buf_len <= 0)
		m_buf_len = 1024;

	m_pBuffer = new char[m_buf_len];
	if (m_pBuffer)
	{
		ACE_OS::setpwent();
		if (::getpwuid_r(uid,&m_pwd2,m_pBuffer,m_buf_len,&m_pwd) != 0)
			m_pwd = 0;

		ACE_OS::endpwent();
	}
}

Root::pw_info::~pw_info()
{
	delete [] m_pBuffer;
}

Root::SpawnedProcess::SpawnedProcess() :
m_pid(ACE_INVALID_PID)
{
}

Root::SpawnedProcess::~SpawnedProcess()
{
	if (m_pid != ACE_INVALID_PID)
	{
		pid_t retv = 0;
		for (int i=0;i<10;++i)
		{
			ACE_exitcode ec;
			retv = ACE_OS::wait(m_pid,&ec,WNOHANG);
			if (retv != 0)
				break;

			ACE_OS::sleep(1);
			retv = 0;
		}

		if (retv == 0)
			ACE_OS::kill(m_pid,SIGKILL);

		m_pid = ACE_INVALID_PID;
	}
}

extern char **environ;

#define SAFE_PATH "/usr/local/bin:/usr/bin:/bin"

bool Root::SpawnedProcess::CleanEnvironment()
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
	char** cleanenv = static_cast<char**>(ACE_OS::calloc(env_max,sizeof(char*)));
	if (!cleanenv)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("ACE_OS::calloc() failed!")),false);

	char pathbuf[PATH_MAX + 6];
	ACE_OS::snprintf(pathbuf,PATH_MAX + 6,"PATH=%s",SAFE_PATH);
	cleanenv[0] = ACE_OS::strdup(pathbuf);

	size_t cidx = 1;
	for (char** ep = environ; *ep && cidx < env_max-1; ep++)
	{
		if (!ACE_OS::strncmp(*ep,"OMEGA_",6)==0)
		{
			cleanenv[cidx] = *ep;
			cidx++;
		}
		else
		{
			for (size_t idx = 0; safe_env_lst[idx]; idx++)
			{
				if (ACE_OS::strncmp(*ep,safe_env_lst[idx],ACE_OS::strlen(safe_env_lst[idx]))==0)
				{
					cleanenv[cidx] = *ep;
					cidx++;
					break;
				}
			}
		}
	}

	cleanenv[cidx] = NULL;
	environ = cleanenv;

	return true;
}

bool Root::SpawnedProcess::close_all_fds()
{
#ifdef LINUX
	/* faster under linux as avoid untold syscalls */
	return linux_close_all_fds();

#elif defined(_POSIX_OPEN_MAX)
	/* POSIX's way, needs testing on other unix systems particularly solaris */
	#if (_POSIX_OPEN_MAX == -1)
		/* value unsupported */

	#elif (_POSIX_OPEN_MAX == 0)
		/* value only obtainable at runtime */
		return posix_close_all_fds(sysconf(_SC_OPEN_MAX));
	#else
		/* value statically defined */
		return posix_close_all_fds(_POSIX_OPEN_MAX);
	#endif

#else
	#error Not linux and not POSIX compliant, enable a system specific way
#endif /* def LINUX */
}

bool Root::SpawnedProcess::posix_close_all_fds(long max_fd)
{
	long x =3;

	/* this is seriously slow as involves SC_OPEN_MAX close() syscalls
	most of which were never opened */
	if (max_fd <= 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: Indeterminate limit passed in, pick a high limit and pass that to %s\n"),__func__),false);

	while(x <= max_fd)
		close(x++);

	return true;
}

/* this might work for a mac too */
bool Root::SpawnedProcess::linux_close_all_fds()
{
	/* should have nothing like this many file descriptors open */
	const int MAX_N_FDS = 100;
	int fds[MAX_N_FDS] = {0};

	int count = -1;
	DIR *pdir;
	struct dirent *pfile;

	char str[1024] = {0};
	snprintf(str,1024,"/proc/%u/fd/",getpid());

again:
	/* walk proc filesystem and close fds there */
	errno = 0;
	pdir = opendir(str);
	if(!pdir)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("opendir() failed!")),false);

	while ((pfile = readdir(pdir)) != 0)
	{
		/* skip ./ and ../ entries */
		if ('.' == *pfile->d_name)
			continue;

		/* skip stdin */
		if (!strcmp("0",pfile->d_name))
			continue;

		/* skip stderr */
		if (!strcmp("1",pfile->d_name))
			continue;

		/* skip stdout */
		if (!strcmp("2",pfile->d_name))
			continue;

		/* close all existing entries and restart loop */

		void* FIXME_JAY;

		/* Commented out becaus eit doesn't build
		if (++count >= entries)
		{
			while(--count >= 0 )
			{
				// FIXME add proper error handling
				close(fds[x++]);
			}

			closedir(pdir);
			pdir  = NULL;
			pfile = NULL;
			goto again;
		}*/
		fds[count] = atoi(pfile->d_name);
	}

	void* TODO; // Check this!

	closedir(pdir);

	return true;
}

bool Root::SpawnedProcess::LogonSandboxUser(user_id_type& uid)
{
	void* TICKET_96; // Look at using PAM for this...

	// Get the correct uid from the registry
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Thread_Mutex> reg_root = Manager::get_registry();

	// Get the uid...
	ACE_INT64 key = 0;
	if (reg_root->open_key(key,"Server\\Sandbox",0) != 0)
		return true;

	ACE_CDR::LongLong sb_uid = (ACE_CDR::ULong)-1;
	int err = reg_root->get_integer_value(key,"Uid",0,sb_uid);
	if (err != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: Failed to get sandbox uid from registry: %C"),ACE_OS::strerror(err)),false);

	uid = static_cast<uid_t>(sb_uid);
	return true;
}

void Root::SpawnedProcess::CloseSandboxLogon(user_id_type /*uid*/)
{
}

bool Root::SpawnedProcess::Spawn(uid_t uid, const ACE_CString& strPipe, bool bSandbox)
{
	m_bSandbox = bSandbox;

	// Check our uid
	bool bUnsafeStart = false;
	uid_t our_uid = ACE_OS::getuid();
	if (our_uid != 0)
	{
		if (!unsafe_sandbox())
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("OOServer must be started as a root.\n")),false);
		else if (our_uid == uid)
			bUnsafeStart = true;
		else
		{
			Root::pw_info pw(our_uid);
			if (!pw)
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("getpwuid() failed!")),false);

			const char msg[] =
				"OOServer is running under a user account that does not have the priviledges required to fork and setuid as a different user.\n\n"
				"Because the 'Unsafe' value is set in the registry, the new user process will be started under the user account '%s'\n\n"
				"This is a security risk, and should only be allowed for debugging purposes, and only then if you really know what you are doing.";

			char szBuf[1024];
			ACE_OS::snprintf(szBuf,1024,msg,pw->pw_name);

			// Prompt for continue...
			ACE_ERROR((LM_WARNING,L"%s",szBuf));
#if defined(OMEGA_DEBUG)
			ACE_OS::printf("\n\nDo you want to allow this? [Y/N/D(ebug)]: ");
#else
			ACE_OS::printf("\n\nDo you want to allow this? [Y/N]: ");
#endif
			ACE_OS::fflush(stdout);

			char szIn[2];
			ACE_OS::fgets(szIn,2,stdin);

#if defined(OMEGA_DEBUG)
			if (szIn[0] == 'D' || szIn[0] == 'd')
				AttachDebugger(ACE_OS::getpid());
			else
#endif
				if (szIn[0] != 'Y' && szIn[0] != 'y')
					return false;

			ACE_OS::printf("\nYou chose to continue... on your head be it!\n\n");
			ACE_OS::fflush(stdout);

			bUnsafeStart = true;
		}
	}

	pid_t child_id = ACE_OS::fork();
	if (child_id == -1)
	{
		// Error
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("ACE_OS::fork() failed!")),false);
	}
	else if (child_id == 0)
	{
		// We are the child...

		// get our pw_info
		Root::pw_info pw(uid);
		if (!pw)
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("getpwuid() failed!")));
			ACE_OS::exit(errno);
		}

		if (!bUnsafeStart)
		{
			// Set our gid...
			if (ACE_OS::setgid(pw->pw_gid) != 0)
			{
				ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("setgid() failed!")));
				ACE_OS::exit(errno);
			}

			// Init our groups...
			if (initgroups(pw->pw_name,pw->pw_gid) != 0)
			{
				ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("initgroups() failed!")));
				ACE_OS::exit(errno);
			}

			// Stop being priviledged!
			if (ACE_OS::setuid(uid) != 0)
			{
				ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("setuid() failed!")));
				ACE_OS::exit(errno);
			}
		}

		// Close all open handles
		if (!close_all_fds())
			ACE_OS::exit(errno);

		// Clean up environment...
		if (!CleanEnvironment())
			ACE_OS::exit(errno);

		// This all needs sorting out badly!
        void* TODO;

		// mount and chroot into our own FUSE dir


		// Exec the user process
		char* cmd_line[] =
		{
		    "oosvruser", //  argv[0] = Process name
		    0,           //  argv[1] = Pipe name
		    0
		};
		cmd_line[1] = const_cast<char*>(ACE_TEXT_CHAR_TO_TCHAR(strPipe.c_str()));

		int err = ACE_OS::execv("./oosvruser",cmd_line);

		ACE_ERROR((LM_WARNING,ACE_TEXT("Child process exiting with code: %d\n"),err));

		ACE_OS::exit(err);
	}
	else
	{
		// We are the parent...
		m_uid = uid;
		m_pid = child_id;
	}

	return true;
}

bool Root::SpawnedProcess::CheckAccess(const char* pszFName, ACE_UINT32 mode, bool& bAllowed)
{
	bAllowed = false;

	// Get file info
	ACE_stat sb;
	if (ACE_OS::stat(pszFName,&sb) != 0)
		return false;

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
		Root::pw_info pw(m_uid);
		if (!pw)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("getpwuid() failed!")),false);

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

bool Root::SpawnedProcess::InstallSandbox(int argc, ACE_TCHAR* argv[])
{
	ACE_CString strUName = "omega_sandbox";
	if (argc>=1)
		strUName = ACE_TEXT_ALWAYS_CHAR(argv[0]);

	ACE_OS::setpwent();
	passwd* pw = ACE_OS::getpwnam(strUName.c_str());
	if (!pw)
	{
		if (errno)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("getpwnam() failed!")),false);
		else
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("You must add a user account for 'omega_sandbox' or supply a valid user name on the command line\n")),false);
	}
	ACE_OS::endpwent();

	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Thread_Mutex> reg_root = Manager::get_registry();

	// Set the sandbox uid
	ACE_INT64 key = 0;
	if (Manager::get_registry()->create_key(key,"Server\\Sandbox",false,0,0) != 0)
		return false;

	int err = reg_root->set_integer_value(key,"Uid",0,pw->pw_uid);
	if (err != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: Failed to set sandbox uid in registry: %C"),ACE_OS::strerror(err)),false);

	return true;
}

bool Root::SpawnedProcess::UninstallSandbox()
{
	return true;
}

bool Root::SpawnedProcess::SecureFile(const ACE_CString& strFilename)
{
	// Make sure the file is owned by root (0)
	if (chown(strFilename.c_str(),0,(gid_t)-1) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("chown failed")),false);

	if (chmod(strFilename.c_str(),S_IRWXU) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("chmod failed")),false);

	return true;
}

bool Root::SpawnedProcess::Compare(user_id_type uid)
{
	return (m_uid == uid);
}

bool Root::SpawnedProcess::IsSameUser(user_id_type uid)
{
	return Compare(uid);
}

ACE_CString Root::SpawnedProcess::GetRegistryHive()
{
	ACE_CString strDir;
	if (m_bSandbox)
		strDir = "/var/lib/omegaonline";
	else
	{
		pw_info pw(m_uid);
		if (!pw)
			return "";

		strDir = pw->pw_dir;
		strDir += "/.omegaonline";
	}

	if (ACE_OS::mkdir(strDir.c_str(),S_IRWXU | S_IRWXG | S_IROTH) != 0)
	{
		int err = ACE_OS::last_error();
		if (err != EEXIST)
			return "";
	}

	return strDir + "/user.regdb";
}

bool Root::SpawnedProcess::unsafe_sandbox()
{
	// Get the user name and pwd...
	ACE_INT64 key = 0;
	if (Manager::get_registry()->open_key(key,"Server\\Sandbox",0) != 0)
		return false;

	ACE_CDR::LongLong v = 0;
	if (Manager::get_registry()->get_integer_value(key,"Unsafe",0,v) != 0)
	{
#if defined(OMEGA_DEBUG)
		return true;
#else
		return false;
#endif
	}

	return (v == 1);
}

#endif // !ACE_WIN32
