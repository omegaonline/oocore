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
#include "./RootManager.h"

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
int UserMain(u_short uPort);

bool Root::SpawnedProcess::Spawn(uid_t uid, u_short uPort, ACE_CString& strSource)
{
	pid_t child_id = ACE_OS::fork();
	if (child_id == -1)
	{
		// Error
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("ACE_OS::fork() failed!")));
		return false;
	}
	else if (child_id == 0)
	{
		// We are the child...

		// TODO: Set the correct user id to uid, init the groups and clean up the environment variables...
		#error TODO!
		/*
		ACE_OS::setuid(uid);
		ACE_OS::setregid();
		ACE_OS::setgid
		init_groups();

		// Do something with environment...
		getpwuid();
		*/

		// Now just run UserMain and exit
		ACE_OS::exit(UserMain(uPort));
		return true;
	}
	else
	{
		// We are the parent...
		m_uid = uid;
		m_pid = child_id;
		return true;
	}
}

bool Root::SpawnedProcess::IsRunning()
{
	if (m_pid == ACE_INVALID_PID)
		return false;
	
	return (ACE_OS::kill(m_pid,0) == 0 || errno != ESRCH);
}

bool Root::SpawnedProcess::CheckAccess(const char* pszFName, ACE_UINT32 mode, bool& bAllowed)
{
	int rc = 0;
	ACE_stat sb;

	// Get file info
	if (ACE_OS::stat(pszFName,&sb) != 0)
		return false;

	if (mode==O_RDONLY && (sb.st_mode & S_IROTH))
		return true;
	else if (mode==O_WRONLY && (sb.st_mode & S_IWOTH))
		return true;
	else if (mode==O_RDWR && (sb.st_mode & (S_IROTH | S_IWOTH)))
		return true;

	// Is the supplied user the file's owner
	if (sb.st_uid == m_uid)
	{
		if (mode==O_RDONLY && (sb.st_mode & S_IRUSR))
			return true;
		else if (mode==O_WRONLY && (sb.st_mode & S_IWUSR))
			return true;
		else if (mode==O_RDWR && (sb.st_mode & (S_IRUSR | S_IWUSR)))
			return true;
	}
	
	// Get the suppied user's group see if that is the same as the file's group
	Root::pw_info pw(m_uid);
	if (!pw)
		return false;

	// Is the file's gid the same as the specified user's
	if (pw->pw_gid == sb.st_gid)
	{
		if (mode==O_RDONLY && (sb.st_mode & S_IRGRP))
			return true;
		else if (mode==O_WRONLY && (sb.st_mode & S_IWGRP))
			return true;
		else if (mode==O_RDWR && (sb.st_mode & (S_IRGRP | S_IWGRP)))
			return true;	
	}		

	return false;
}

bool Root::SpawnedProcess::ResolveTokenToUid(uid_t token, ACE_CString& uid, ACE_CString& strSource)
{
	// Get the suppied user's group see if that is the same as the file's group
	Root::pw_info pw(token);
	if (!pw)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("::getpwuid() failed!")));
		strSource = "Root::SpawnedProcess::ResolveTokenToUid - getpwuid";
		return false;
	}

	// Return the username
	uid = pw->pw_name;
	return true;
}

bool Root::SpawnedProcess::GetSandboxUid(ACE_CString& uid)
{
	ACE_Configuration_Heap& reg_root = Manager::get_registry();

	// Open the server section
	ACE_Configuration_Section_Key sandbox_key;
	if (reg_root.open_section(reg_root.root_section(),ACE_TEXT("Server\\Sandbox"),0,sandbox_key)!=0)
		return false;

	// Get the sandbox uid...
	uid_t uid_sandbox;
	if (reg_root.get_integer_value(sandbox_key,ACE_TEXT("Uid"),uid_sandbox) != 0)
		return false;
	
	// Resolve it...
	ACE_CString strSource;
	if (!ResolveTokenToUid(uid_sandbox,uid,strSource))
		return false;

	// Append a extra to the uid
	uid += "_SANDBOX";

	return true;
}

bool Root::SpawnedProcess::InstallSandbox()
{
	ACE_Configuration_Heap& reg_root = Manager::get_registry();

	// Create the server section
	ACE_Configuration_Section_Key sandbox_key;
	if (reg_root.open_section(reg_root.root_section(),ACE_TEXT("Server\\Sandbox"),1,sandbox_key)!=0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to create Server\\Sandbox key in registry")),false);

	#error TODO:  Set up a sandbox user!

	bool bAddedUser = false;
	uid_t uid_sandbox;
	
	// Set the user name and pwd...
	if (reg_root.set_integer_value(sandbox_key,ACE_TEXT("Uid"),uid_sandbox) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to set sandbox uid in registry")),false);

	if (bAddedUser)
		reg_root.set_integer_value(sandbox_key,ACE_TEXT("AutoAdded"),1);
}

bool Root::SpawnedProcess::UninstallSandbox()
{
    ACE_Configuration_Heap& reg_root = Manager::get_registry();

	// Open the server section
	ACE_Configuration_Section_Key sandbox_key;
	if (reg_root.open_section(reg_root.root_section(),ACE_TEXT("Server\\Sandbox"),0,sandbox_key)!=0)
		return true;

	// Get the user name and pwd...
	uid_t uid_sandbox;
	if (reg_root.get_integer_value(sandbox_key,ACE_TEXT("Uid"),uid_sandbox) != 0)
		return true;

	u_int bUserAdded = 0;
	reg_root.get_integer_value(sandbox_key,ACE_TEXT("AutoAdded"),bUserAdded);
	if (bUserAdded)
	{
		#error TODO:  Remove the user...
	}

	return true;
}

#endif // !ACE_WIN32
