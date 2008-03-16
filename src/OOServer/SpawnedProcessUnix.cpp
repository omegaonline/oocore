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

// Forward declare UserMain
int UserMain(const ACE_TString& strPipe);

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
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: %p\n",L"ACE_OS::calloc() failed!"),false);

	char pathbuf[PATH_MAX + 6];
	sprintf(pathbuf,"PATH=%s",SAFE_PATH);
	cleanenv[0] = strdup(pathbuf);

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

bool Root::SpawnedProcess::LogonSandboxUser(user_id_type& uid)
{
	void* TODO; // Look at using PAM for this...

	// Get the correct uid from the registry
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> reg_root = Manager::get_registry();

	// Get the uid...
	ACE_CDR::ULong sb_uid = (ACE_CDR::ULong)-1;
	int err = reg_root->get_integer_value(L"Server\\Sandbox",L"Uid",sb_uid);
	if (err != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: Failed to get sandbox uid from registry: %s\n",ACE_OS::strerror(err)),false);

	uid = static_cast<uid_t>(sb_uid);
	return true;
}

void Root::SpawnedProcess::CloseSandboxLogon(user_id_type /*uid*/)
{
}

bool Root::SpawnedProcess::Spawn(uid_t uid, const ACE_TString& strPipe, bool /*bSandbox*/)
{
	pid_t child_id = ACE_OS::fork();
	if (child_id == -1)
	{
		// Error
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: %p\n",L"ACE_OS::fork() failed!"),false);
	}
	else if (child_id == 0)
	{
		// We are the child...

		// get our pw_info
		Root::pw_info pw(uid);
		if (!pw)
		{
			ACE_ERROR((LM_ERROR,L"%N:%l: %p\n",L"::getpwuid() failed!"));
			ACE_OS::exit(errno);
		}

		// Set our gid...
		if (ACE_OS::setgid(pw->pw_gid) != 0)
		{
			ACE_ERROR((LM_ERROR,L"%N:%l: %p\n",L"setgid() failed!"));
			ACE_OS::exit(errno);
		}

		// Init our groups...
		if (::initgroups(pw->pw_name,pw->pw_gid) != 0)
		{
			ACE_ERROR((LM_ERROR,L"%N:%l: %p\n",L"::initgroups() failed!"));
			ACE_OS::exit(errno);
		}

		// Stop being priviledged!
		if (!ACE_OS::setuid(uid) != 0)
		{
			ACE_ERROR((LM_ERROR,L"%N:%l: %p\n",L"setuid() failed!"));
			ACE_OS::exit(errno);
		}

		// Clean up environment...
		if (!CleanEnvironment())
			ACE_OS::exit(errno);

		// Set cwd
		if (ACE_OS::chdir(pw->pw_dir) != 0)
		{
			ACE_ERROR((LM_ERROR,L"%N:%l: %p\n",L"chdir() failed!"));
			ACE_OS::exit(errno);
		}

		// Now just run UserMain and exit
		int err = UserMain(strPipe);

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

bool Root::SpawnedProcess::CheckAccess(const wchar_t* pszFName, ACE_UINT32 mode, bool& bAllowed)
{
	bAllowed = false;

	// Get file info
	ACE_stat sb;
	if (ACE_OS::stat(pszFName,&sb) != 0)
		return false;

	if (mode==O_RDONLY && (sb.st_mode & S_IROTH))
	{
		bAllowed = true;
		return true;
	}
	else if (mode==O_WRONLY && (sb.st_mode & S_IWOTH))
	{
		bAllowed = true;
		return true;
	}
	else if (mode==O_RDWR && (sb.st_mode & (S_IROTH | S_IWOTH)))
	{
		bAllowed = true;
		return true;
	}

	// Is the supplied user the file's owner
	if (sb.st_uid == m_uid)
	{
		if (mode==O_RDONLY && (sb.st_mode & S_IRUSR))
		{
			bAllowed = true;
			return true;
		}
		else if (mode==O_WRONLY && (sb.st_mode & S_IWUSR))
		{
			bAllowed = true;
			return true;
		}
		else if (mode==O_RDWR && (sb.st_mode & (S_IRUSR | S_IWUSR)))
		{
			bAllowed = true;
			return true;
		}
	}

	// Get the suppied user's group see if that is the same as the file's group
	Root::pw_info pw(m_uid);
	if (!pw)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: %p\n",L"::getpwuid() failed!"),false);

	// Is the file's gid the same as the specified user's
	if (pw->pw_gid == sb.st_gid)
	{
		if (mode==O_RDONLY && (sb.st_mode & S_IRGRP))
		{
			bAllowed = true;
			return true;
		}
		else if (mode==O_WRONLY && (sb.st_mode & S_IWGRP))
		{
			bAllowed = true;
			return true;
		}
		else if (mode==O_RDWR && (sb.st_mode & (S_IRGRP | S_IWGRP)))
		{
			bAllowed = true;
			return true;
		}
	}

	return true;
}

bool Root::SpawnedProcess::InstallSandbox(int argc, wchar_t* argv[])
{
	ACE_CString strUName = "omega_sandbox";
	if (argc>=1)
		strUName = ACE_Wide_To_Ascii(argv[0]).char_rep();

	ACE_OS::setpwent();
	passwd* pw = ACE_OS::getpwnam(strUName.c_str());
	if (!pw)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: %p\n",L"getpwnam() failed!"),false);
	ACE_OS::endpwent();

	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Null_Mutex> reg_root = Manager::get_registry();

	// Set the sandbox uid
	int err = reg_root->set_integer_value(L"Server\\Sandbox",L"Uid",pw->pw_uid);
	if (err != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l: Failed to set sandbox uid in registry: %s\n",ACE_OS::strerror(err)),false);

	return true;
}

bool Root::SpawnedProcess::UninstallSandbox()
{
	return (Manager::get_registry().delete_value(L"Server\\Sandbox",L"Uid") == 0);
}

ACE_CString Root::SpawnedProcess::get_home_dir()
{
	pw_info pw(ACE_OS::getuid());
	if (!pw)
		return "";

	return pw->pw_dir;
}

bool Root::SpawnedProcess::SecureFile(const ACE_TString& strFilename)
{
	ACE_CString strShortName = ACE_Wide_To_Ascii(strFilename.c_str()).char_rep();

	// Make sure the file is owned by root (0)
	if (::chown(strShortName.c_str(),0,(gid_t)-1) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"chown failed"),false);

	void* TODO; // Check the group permissions...

	if (::chmod(strShortName.c_str(),S_IRWXU | S_IRWXG | S_IROTH) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"chmod failed"),false);

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

ACE_TString Root::SpawnedProcess::GetRegistryHive()
{
	ACE_TString strDir;
	if (bSandbox)
		strDir = L"/var/lib/omegaonline";
	else
		strDir = ACE_Ascii_To_Wide((Root::SpawnedProcess::get_home_dir() + "/.omegaonline").c_str()).wchar_rep();

	if (ACE_OS::mkdir(strDir.c_str(),S_IRWXU | S_IRWXG | S_IROTH) != 0)
	{
		int err = ACE_OS::last_error();
		if (err != EEXIST)
			return -1;
	}

	ACE_TString strRegistry = strDir + L"/user.regdb";
}

#endif // !ACE_WIN32
