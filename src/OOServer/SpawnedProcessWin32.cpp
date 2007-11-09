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
#include "./SpawnedProcess.h"

#if defined(ACE_WIN32)

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0500
#elif _WIN32_WINNT < 0x0500
#error OOServer requires _WIN32_WINNT >= 0x0500!
#endif

#include <userenv.h>
#include <lm.h>
#include <sddl.h>
#include <ntsecapi.h>
#include <aclapi.h>

#if !defined(OMEGA_WIDEN_STRING)
#define OMEGA_WIDEN_STRING_II(STRING) L ## STRING
#define OMEGA_WIDEN_STRING_I(STRING)  OMEGA_WIDEN_STRING_II(STRING)
#define OMEGA_WIDEN_STRING(STRING)    OMEGA_WIDEN_STRING_I(STRING)
#endif

#define LOG_FAILURE(err) LogFailure((err),OMEGA_WIDEN_STRING(__FILE__),__LINE__)

#ifndef PROTECTED_DACL_SECURITY_INFORMATION
#define PROTECTED_DACL_SECURITY_INFORMATION     (0x80000000L)
#endif

Root::SpawnedProcess::SpawnedProcess() :
	m_hToken(NULL),
	m_hProfile(NULL),
	m_hProcess(NULL)
{
}

Root::SpawnedProcess::~SpawnedProcess()
{
	if (m_hProcess)
	{
		DWORD dwWait = 5000;
		DWORD dwRes = WaitForSingleObject(m_hProcess,dwWait);
		if (dwRes != WAIT_OBJECT_0)
			TerminateProcess(m_hProcess,UINT(-1));

		CloseHandle(m_hProcess);
	}

	if (m_hProfile)
		UnloadUserProfile(m_hToken,m_hProfile);

	if (m_hToken)
		CloseHandle(m_hToken);
}

DWORD Root::SpawnedProcess::LoadUserProfileFromToken(HANDLE hToken, HANDLE& hProfile)
{
    int err = 0;

	// Find out all about the user associated with hToken
	DWORD dwBufSize = 0;
	GetTokenInformation(hToken,TokenUser,NULL,0,&dwBufSize);
	if (dwBufSize == 0)
	{
		DWORD err = GetLastError();
		LOG_FAILURE(err);
		return err;
	}

	PTOKEN_USER pUserInfo = (PTOKEN_USER)ACE_OS::malloc(dwBufSize);
	if (!pUserInfo)
	{
		LOG_FAILURE(ERROR_OUTOFMEMORY);
		return ERROR_OUTOFMEMORY;
	}

	memset(pUserInfo,0,dwBufSize);
	if (!GetTokenInformation(hToken,TokenUser,pUserInfo,dwBufSize,&dwBufSize))
	{
		int err = GetLastError();
		ACE_OS::free(pUserInfo);
		LOG_FAILURE(err);
		return err;
	}

	// Get the names associated with the user SID
	ACE_WString strUserName;
	ACE_WString strDomainName;

	SID_NAME_USE name_use;
	DWORD dwUNameSize = 0;
	DWORD dwDNameSize = 0;
	LookupAccountSidW(NULL,pUserInfo->User.Sid,NULL,&dwUNameSize,NULL,&dwDNameSize,&name_use);
	if (dwUNameSize == 0)
	{
		err = GetLastError();
		LOG_FAILURE(err);
	}
    else
	{
		LPWSTR pszUserName = 0;
		ACE_NEW_NORETURN(pszUserName,wchar_t[dwUNameSize]);
        if (!pszUserName)
		{
            err = ERROR_OUTOFMEMORY;
			LOG_FAILURE(err);
		}
        else
        {
            LPWSTR pszDomainName = NULL;
            if (dwDNameSize)
            {
				ACE_NEW_NORETURN(pszDomainName,wchar_t[dwDNameSize]);
                if (!pszDomainName)
				{
					err = ERROR_OUTOFMEMORY;
					LOG_FAILURE(err);
				}
            }

            if (err == 0)
            {
                if (!LookupAccountSidW(NULL,pUserInfo->User.Sid,pszUserName,&dwUNameSize,pszDomainName,&dwDNameSize,&name_use))
				{
                    err = GetLastError();
					LOG_FAILURE(err);
				}
                else
                {
                    strUserName = pszUserName;
                    strDomainName = pszDomainName;
                }
            }
            delete [] pszDomainName;
            delete [] pszUserName;
        }
	}

	// Done with user info
	ACE_OS::free(pUserInfo);

    if (err != 0)
        return err;

	// Lookup a DC for pszDomain
    ACE_WString strDCName;
    LPWSTR pszDCName = NULL;
    if (NetGetAnyDCName(NULL,strDomainName.is_empty() ? NULL : strDomainName.c_str(),(LPBYTE*)&pszDCName) == NERR_Success)
    {
        strDCName = pszDCName;
        NetApiBufferFree(pszDCName);
    }

    // Try to find the user's profile path...
    ACE_WString strProfilePath;
    USER_INFO_3* pInfo = NULL;
    if (NetUserGetInfo(strDCName.is_empty() ? NULL : strDCName.c_str(),strUserName.c_str(),3,(LPBYTE*)&pInfo) == NERR_Success)
    {
        if (pInfo->usri3_profile)
            strProfilePath = pInfo->usri3_profile;

        NetApiBufferFree(pInfo);
    }

	// Load the Users Profile
	PROFILEINFOW profile_info;
	ACE_OS::memset(&profile_info,0,sizeof(profile_info));
    profile_info.dwSize = sizeof(PROFILEINFOW);
	profile_info.dwFlags = PI_NOUI;
	profile_info.lpUserName = (WCHAR*)strUserName.c_str();
	if (!strProfilePath.empty())
		profile_info.lpProfilePath = (WCHAR*)strProfilePath.c_str();
	if (!strDCName.empty())
		profile_info.lpServerName = (WCHAR*)strDCName.c_str();

	BOOL bSuccess = LoadUserProfileW(hToken,&profile_info);

	if (!bSuccess)
	{
		DWORD dwErr = GetLastError();
		LOG_FAILURE(dwErr);
		return dwErr;
	}

	hProfile = profile_info.hProfile;
	return ERROR_SUCCESS;
}

DWORD Root::SpawnedProcess::SpawnFromToken(HANDLE hToken, const ACE_WString& strPipe, bool bLoadProfile)
{
	// Get our module name
	WCHAR szPath[MAX_PATH];
	if (!GetModuleFileNameW(NULL,szPath,MAX_PATH))
	{
		DWORD dwErr = GetLastError();
		LOG_FAILURE(dwErr);
		return dwErr;
	}

	// Load up the users profile
	HANDLE hProfile = NULL;
	DWORD dwRes = 0;
	if (bLoadProfile)
	{
		dwRes = LoadUserProfileFromToken(hToken,hProfile);
		if (dwRes != ERROR_SUCCESS)
			return dwRes;
	}

	// Load the Users Environment vars
	LPVOID lpEnv = NULL;
	if (!CreateEnvironmentBlock(&lpEnv,hToken,FALSE))
	{
		dwRes = GetLastError();
		if (hProfile)
			UnloadUserProfile(hToken,hProfile);
		LOG_FAILURE(dwRes);
		return dwRes;
	}

	// Get the primary token from the impersonation token
	HANDLE hPriToken = 0;
	if (!DuplicateTokenEx(hToken,TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,NULL,SecurityImpersonation,TokenPrimary,&hPriToken))
	{
		dwRes = GetLastError();
		DestroyEnvironmentBlock(lpEnv);
		if (hProfile)
			UnloadUserProfile(hToken,hProfile);
		LOG_FAILURE(dwRes);
		return dwRes;
	}	

	// Init our startup info
	STARTUPINFOW startup_info;
	ACE_OS::memset(&startup_info,0,sizeof(startup_info));
	startup_info.cb = sizeof(STARTUPINFOW);
	startup_info.lpDesktop = L"";

	DWORD dwFlags = CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT;

	ACE_WString strCmdLine = L"\"" + ACE_WString(szPath) + L"\" --spawned " + strPipe;

	// Actually create the process!
	PROCESS_INFORMATION process_info;

#ifdef OMEGA_DEBUG
	if (IsDebuggerPresent())
	{
		strCmdLine += L" --break";

		if (!CreateProcessAsUserW(hPriToken,NULL,(wchar_t*)strCmdLine.c_str(),NULL,NULL,FALSE,dwFlags,lpEnv,NULL,&startup_info,&process_info))
		{
			dwRes = GetLastError();
			CloseHandle(hPriToken);
			DestroyEnvironmentBlock(lpEnv);
			if (hProfile)
				UnloadUserProfile(hToken,hProfile);
			LOG_FAILURE(dwRes);
			return dwRes;
		}
	}
	else
#endif
	
	if (!CreateProcessAsUserW(hPriToken,NULL,(wchar_t*)strCmdLine.c_str(),NULL,NULL,FALSE,dwFlags,lpEnv,NULL,&startup_info,&process_info))
	{
		dwRes = GetLastError();
		CloseHandle(hPriToken);
		DestroyEnvironmentBlock(lpEnv);
		if (hProfile)
			UnloadUserProfile(hToken,hProfile);
		LOG_FAILURE(dwRes);
		return dwRes;
	}

	// Done with primary token
	CloseHandle(hPriToken);

	// Done with environment block...
	DestroyEnvironmentBlock(lpEnv);

	// Stash handles to close on end...
	m_hProfile = hProfile;
	m_hProcess = process_info.hProcess;
	m_hToken = hToken;

	// And close any others
	CloseHandle(process_info.hThread);

	return ERROR_SUCCESS;
}

bool Root::SpawnedProcess::unsafe_sandbox()
{
	// Get the local machine registry
	ACE_Configuration_Heap& reg_root = Manager::get_registry();

	// Get the server section
	ACE_Configuration_Section_Key sandbox_key;
	if (reg_root.open_section(reg_root.root_section(),L"Server\\Sandbox",0,sandbox_key) != 0)
		return false;

	// Get the user name and pwd...
	u_int v = 0;
	if (reg_root.get_integer_value(sandbox_key,L"Unsafe",v) != 0)
		return IsDebuggerPresent() ? true : false;

	return (v == 1);
}

bool Root::SpawnedProcess::Spawn(Manager::user_id_type id, const ACE_WString& strPipe)
{
	HANDLE hToken = INVALID_HANDLE_VALUE;

	bool bSandbox = (id == static_cast<Manager::user_id_type>(0));
	if (bSandbox)
	{
		if (!LogonSandboxUser(&hToken))
			return false;
	}
	else
		hToken = id;
	
	DWORD dwRes = SpawnFromToken(hToken,strPipe,!bSandbox);
	if (dwRes != ERROR_SUCCESS)
	{
		if (dwRes == ERROR_PRIVILEGE_NOT_HELD && bSandbox && unsafe_sandbox())
		{
			CloseHandle(hToken);
			if (OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,&hToken))
			{
				dwRes = SpawnFromToken(hToken,strPipe,!bSandbox);
				if (dwRes == ERROR_SUCCESS)
					ACE_OS::printf("RUNNING WITH SANDBOX LOGGED IN AS ROOT!\n");
			}
		}
	}

	if (bSandbox)
		CloseHandle(hToken);
	
	return (dwRes == ERROR_SUCCESS);
}

bool Root::SpawnedProcess::LogonSandboxUser(HANDLE* phToken)
{
	// Get the local machine registry
	ACE_Configuration_Heap& reg_root = Manager::get_registry();

	// Get the server section
	ACE_Configuration_Section_Key sandbox_key;
	if (reg_root.open_section(reg_root.root_section(),L"Server\\Sandbox",0,sandbox_key) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Failed to open sandbox key in registry"),false);

	// Get the user name and pwd...
	ACE_WString strUName;
	ACE_WString strPwd;
	if (reg_root.get_string_value(sandbox_key,L"UserName",strUName) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Failed to read sandbox username from registry"),false);

	reg_root.get_string_value(sandbox_key,L"Password",strPwd);

	if (!LogonUserW((LPWSTR)strUName.c_str(),NULL,(LPWSTR)strPwd.c_str(),LOGON32_LOGON_BATCH,LOGON32_PROVIDER_DEFAULT,phToken))
	{
		DWORD dwErr = GetLastError();
		LOG_FAILURE(dwErr);
		return false;
	}

	return true;
}

bool Root::SpawnedProcess::CheckAccess(const wchar_t* pszFName, ACE_UINT32 mode, bool& bAllowed)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
	DWORD cbNeeded = 0;
	if (!GetFileSecurityW(pszFName,DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,pSD,0,&cbNeeded) && GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
	{
		ACE_OS::set_errno_to_last_error();
		return false;
	}

	pSD = static_cast<PSECURITY_DESCRIPTOR>(ACE_OS::malloc(cbNeeded));
	if (!pSD)
		return false;

	if (!GetFileSecurityW(pszFName,DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,pSD,cbNeeded,&cbNeeded))
	{
		ACE_OS::set_errno_to_last_error();
		return false;
	}

	// Map the generic access rights
	DWORD dwAccessDesired = 0;
	if (mode & O_RDONLY)
		dwAccessDesired = FILE_GENERIC_READ;
	else if (mode & O_WRONLY)
		dwAccessDesired = FILE_GENERIC_WRITE;
	else if (mode & O_RDWR)
		dwAccessDesired = FILE_GENERIC_READ | FILE_GENERIC_WRITE;

	GENERIC_MAPPING generic =
	{
		FILE_GENERIC_READ,
		FILE_GENERIC_WRITE,
		FILE_GENERIC_EXECUTE,
		FILE_ALL_ACCESS
	};

	MapGenericMask(&dwAccessDesired,&generic);

	// Do the access check
	PRIVILEGE_SET privilege_set;
	DWORD dwPrivSetSize = sizeof(privilege_set);
	DWORD dwAccessGranted = 0;
	BOOL bAllowedVal = FALSE;
	BOOL bRes = ::AccessCheck(pSD,m_hToken,dwAccessDesired,&generic,&privilege_set,&dwPrivSetSize,&dwAccessGranted,&bAllowedVal);
	DWORD err = GetLastError();
	ACE_OS::free(pSD);
	
	if (!bRes && err!=ERROR_SUCCESS)
	{
		ACE_OS::last_error(err);
		ACE_OS::set_errno_to_last_error();
		return LOG_FAILURE(err);
	}
	bAllowed = (bAllowedVal ? true : false);
	return true;
}

bool Root::SpawnedProcess::LogFailure(DWORD err, const wchar_t* pszFile, unsigned int nLine)
{
	if (err != (DWORD)-1)
	{
		const wchar_t szDefault[] = L"Unknown error code.";
		LPVOID lpMsgBuf = (LPVOID)szDefault;

		FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPWSTR)&lpMsgBuf,
			0,	NULL);

		ACE_ERROR((LM_ERROR,L"%W:%d [%P:%t] %W\n",pszFile,nLine,(LPWSTR)lpMsgBuf));

		// Free the buffer.
		if (lpMsgBuf != (LPVOID)szDefault)
			LocalFree(lpMsgBuf);
	}

	return false;
}

bool Root::SpawnedProcess::InstallSandbox(int argc, wchar_t* argv[])
{
	ACE_WString strUName = L"_OMEGA_SANDBOX_USER_";
	ACE_WString strPwd = L"4th_(*%LGe895y^$N|2";

	if (argc>=1)
		strUName = argv[0];
	if (argc>=2)
		strPwd = argv[1];

	ACE_Configuration_Heap& reg_root = Manager::get_registry();

	// Create the server section
	ACE_Configuration_Section_Key sandbox_key;
	if (reg_root.open_section(reg_root.root_section(),L"Server\\Sandbox",1,sandbox_key)!=0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Failed to create Server\\Sandbox key in registry"),false);

	USER_INFO_2	info =
	{
		(LPWSTR)strUName.c_str(),  // usri2_name;
		(LPWSTR)strPwd.c_str(),    // usri2_password;
		0,                         // usri2_password_age;
		USER_PRIV_USER,            // usri2_priv;
		NULL,                      // usri2_home_dir;
		L"This account is used by the OmegaOnline sandbox to control access to system resources", // usri2_comment;
		UF_SCRIPT | UF_PASSWD_CANT_CHANGE | UF_DONT_EXPIRE_PASSWD | UF_NORMAL_ACCOUNT, // usri2_flags;
		NULL,                      // usri2_script_path;
		0,                         // usri2_auth_flags;
		L"Omega Online sandbox user account",   // usri2_full_name;
		L"This account is used by the OmegaOnline sandbox to control access to system resources", // usri2_usr_comment;
		0,                         // usri2_parms;
		NULL,                      // usri2_workstations;
		0,                         // usri2_last_logon;
		0,                         // usri2_last_logoff;
		TIMEQ_FOREVER,             // usri2_acct_expires;
		USER_MAXSTORAGE_UNLIMITED, // usri2_max_storage;
		0,                         // usri2_units_per_week;
		NULL,                      // usri2_logon_hours;
		(DWORD)-1,                 // usri2_bad_pw_count;
		(DWORD)-1,                 // usri2_num_logons;
		NULL,                      // usri2_logon_server;
		0,                         // usri2_country_code;
		0,                         // usri2_code_page;
	};
	bool bAddedUser = true;
	NET_API_STATUS err = NetUserAdd(NULL,2,(LPBYTE)&info,NULL);
	if (err == NERR_UserExists)
		bAddedUser = false;
	else if (err != NERR_Success)
		ACE_ERROR_RETURN((LM_ERROR,L"Failed to add sandbox user\n"),false);

	// Now we have to add the SE_BATCH_LOGON_NAME priviledge and remove all the others

	// Lookup the account SID
	DWORD dwSidSize = 0;
	DWORD dwDnSize = 0;
	SID_NAME_USE use;
	LookupAccountNameW(NULL,info.usri2_name,NULL,&dwSidSize,NULL,&dwDnSize,&use);
	if (dwSidSize==0)
	{
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		ACE_ERROR_RETURN((LM_ERROR,L"Failed to lookup account SID\n"),false);
	}

	PSID pSid = static_cast<PSID>(ACE_OS::malloc(dwSidSize));
	if (!pSid)
	{
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		return false;
	}
	wchar_t* pszDName;
	ACE_NEW_NORETURN(pszDName,wchar_t[dwDnSize]);
	if (!pszDName)
	{
		ACE_OS::free(pSid);
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		return false;
	}
	if (!LookupAccountNameW(NULL,info.usri2_name,pSid,&dwSidSize,pszDName,&dwDnSize,&use))
	{
		err = GetLastError();
		ACE_OS::free(pSid);
		delete [] pszDName;
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		ACE_ERROR_RETURN((LM_ERROR,L"Failed to lookup account SID\n"),false);
	}
	delete [] pszDName;

	if (use != SidTypeUser)
	{
		ACE_OS::free(pSid);
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		ACE_ERROR_RETURN((LM_ERROR,L"Bad username\n"),false);
	}

	// Open the local account policy...
	LSA_HANDLE hPolicy;
	LSA_OBJECT_ATTRIBUTES oa;
	memset(&oa,0,sizeof(oa));
	NTSTATUS err2 = LsaOpenPolicy(NULL,&oa,POLICY_ALL_ACCESS,&hPolicy);
	if (err2 != 0)
	{
		ACE_OS::free(pSid);
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		ACE_ERROR_RETURN((LM_ERROR,L"Failed to access policy elements\n"),false);
	}

	LSA_UNICODE_STRING szName;
	szName.Buffer = L"SeBatchLogonRight";
	size_t len = ACE_OS::strlen(szName.Buffer);
	szName.Length = static_cast<USHORT>(len * sizeof(WCHAR));
	szName.MaximumLength = static_cast<USHORT>((len+1) * sizeof(WCHAR));

	err2 = LsaAddAccountRights(hPolicy,pSid,&szName,1);
	if (err2 == 0)
	{
		// Remove all other priviledges
		LSA_UNICODE_STRING* pRightsList;
		ULONG ulRightsCount = 0;
		err2 = LsaEnumerateAccountRights(hPolicy,pSid,&pRightsList,&ulRightsCount);
		if (err2 == 0)
		{
			for (ULONG i=0;i<ulRightsCount;i++)
			{
				if (ACE_OS::strcmp(pRightsList[i].Buffer,L"SeBatchLogonRight") != 0)
				{
					err2 = LsaRemoveAccountRights(hPolicy,pSid,FALSE,&pRightsList[i],1);
					if (err2 != 0)
						break;
				}
			}
		}
	}

	// Done with pSid
	ACE_OS::free(pSid);

	// Done with policy handle
	LsaClose(hPolicy);

	if (err2 != 0)
	{
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		ACE_ERROR_RETURN((LM_ERROR,L"Failed to modify account priviledges\n"),false);
	}

	// Set the user name and pwd...
	if (reg_root.set_string_value(sandbox_key,L"UserName",info.usri2_name) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Failed to set sandbox username in registry"),false);

	if (reg_root.set_string_value(sandbox_key,L"Password",info.usri2_password) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Failed to set sandbox password in registry"),false);

	if (bAddedUser)
		reg_root.set_integer_value(sandbox_key,L"AutoAdded",1);

	return true;
}

bool Root::SpawnedProcess::UninstallSandbox()
{
	ACE_Configuration_Heap& reg_root = Manager::get_registry();

	// Open the server section
	ACE_Configuration_Section_Key sandbox_key;
	if (reg_root.open_section(reg_root.root_section(),L"Server\\Sandbox",0,sandbox_key) == 0)
	{
		// Get the user name and pwd...
		ACE_WString strUName;
		if (reg_root.get_string_value(sandbox_key,L"UserName",strUName) == 0)
		{
			u_int bUserAdded = 0;
			reg_root.get_integer_value(sandbox_key,L"AutoAdded",bUserAdded);
			if (bUserAdded)
				NetUserDel(NULL,strUName.c_str());
		}
	}
	return true;
}

bool Root::SpawnedProcess::SecureFile(const ACE_WString& strFilename)
{
	// Specify the DACL to use.
	// Create a SID for the Users group.
	PSID pSIDUsers = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_USERS,
		0, 0, 0, 0, 0, 0,
		&pSIDUsers))
	{
		return false;
	}

	// Create a SID for the BUILTIN\Administrators group.
	PSID pSIDAdmin = NULL;

	if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pSIDAdmin))
	{
		FreeSid(pSIDUsers);
		return false;
	}

	const int NUM_ACES  = 2;
	EXPLICIT_ACCESSW ea[NUM_ACES];
	ZeroMemory(&ea, NUM_ACES * sizeof(EXPLICIT_ACCESS));

	// Set read access for Users.
	ea[0].grfAccessPermissions = GENERIC_READ;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName = (LPWSTR) pSIDUsers;

	// Set full control for Administrators.
	ea[1].grfAccessPermissions = GENERIC_ALL;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[1].Trustee.ptstrName = (LPWSTR) pSIDAdmin;

	PACL pACL = NULL;
	if (ERROR_SUCCESS != SetEntriesInAclW(NUM_ACES,ea,NULL,&pACL))
	{
		FreeSid(pSIDAdmin);
		FreeSid(pSIDUsers);
		return false;
	}

	FreeSid(pSIDAdmin);
	FreeSid(pSIDUsers);

	// Try to modify the object's DACL.
	DWORD dwRes = SetNamedSecurityInfoW(
		const_cast<LPWSTR>(strFilename.c_str()),         // name of the object
		SE_FILE_OBJECT,              // type of object
		DACL_SECURITY_INFORMATION |  // change only the object's DACL
		PROTECTED_DACL_SECURITY_INFORMATION, // And don't inherit!
		NULL, NULL,                  // don't change owner or group
		pACL,                        // DACL specified
		NULL);                       // don't change SACL

	LocalFree(pACL);

	return (ERROR_SUCCESS == dwRes);
}

void* Root::SpawnedProcess::GetTokenInfo(HANDLE hToken, TOKEN_INFORMATION_CLASS cls)
{
	DWORD dwLen = 0;
	GetTokenInformation(hToken,cls,NULL,0,&dwLen);
	if (dwLen == 0)
		return 0;
		
	void* pBuffer = malloc(dwLen);
	if (!pBuffer)
		return 0;

	if (!GetTokenInformation(hToken,cls,pBuffer,dwLen,&dwLen))
	{
		free(pBuffer);
		return 0;
	}

	return pBuffer;
}

bool Root::SpawnedProcess::MatchSids(ULONG count, PSID_AND_ATTRIBUTES pSids1, PSID_AND_ATTRIBUTES pSids2)
{
	for (ULONG i=0;i<count;++i)
	{
		bool bFound = false;
		for (ULONG j=0;j<count;++j)
		{
			if (EqualSid(pSids1[i].Sid,pSids2[j].Sid) && 
				pSids1[i].Attributes == pSids2[j].Attributes)
			{
				bFound = true;
				break;
			}			
		}

		if (!bFound)
			return false;
	}

	return true;
}

bool Root::SpawnedProcess::MatchPrivileges(ULONG count, PLUID_AND_ATTRIBUTES Privs1, PLUID_AND_ATTRIBUTES Privs2)
{
	for (ULONG i=0;i<count;++i)
	{
		bool bFound = false;
		for (ULONG j=0;j<count;++j)
		{
			if (Privs1[i].Luid.LowPart == Privs2[j].Luid.LowPart && 
				Privs1[i].Luid.HighPart == Privs2[j].Luid.HighPart && 
				Privs1[i].Attributes == Privs2[j].Attributes)
			{
				bFound = true;
				break;
			}			
		}

		if (!bFound)
			return false;
	}

	return true;
}

bool Root::SpawnedProcess::Compare(HANDLE hToken)
{
	TOKEN_GROUPS_AND_PRIVILEGES* pStats1 = static_cast<TOKEN_GROUPS_AND_PRIVILEGES*>(GetTokenInfo(hToken,TokenGroupsAndPrivileges));
	if (!pStats1)
		return false;

	TOKEN_GROUPS_AND_PRIVILEGES* pStats2 = static_cast<TOKEN_GROUPS_AND_PRIVILEGES*>(GetTokenInfo(m_hToken,TokenGroupsAndPrivileges));
	if (!pStats2)
	{
		free(pStats1);
		return false;
	}

	bool bSame = (pStats1->SidCount==pStats2->SidCount && 
		pStats1->RestrictedSidCount==pStats2->RestrictedSidCount && 
		pStats1->PrivilegeCount==pStats2->PrivilegeCount &&
		MatchSids(pStats1->SidCount,pStats1->Sids,pStats2->Sids) &&
		MatchSids(pStats1->RestrictedSidCount,pStats1->RestrictedSids,pStats2->RestrictedSids) &&
		MatchPrivileges(pStats1->PrivilegeCount,pStats1->Privileges,pStats2->Privileges));

	free(pStats1);
	free(pStats2);

	return bSame;
}

#endif // ACE_WIN32
