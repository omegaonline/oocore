/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary and do not use precompiled headers
//
/////////////////////////////////////////////////////////////

#include "./OOServer_Root.h"

#ifdef ACE_WIN32

#include "./SpawnedProcess.h"
#include "./RootManager.h"

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0500
#elif _WIN32_WINNT < 0x0500
#error OOServer requires _WIN32_WINNT >= 0x0500!
#endif

#include <userenv.h>
#include <lm.h>
#include <sddl.h>
#include <ntsecapi.h>

Root::SpawnedProcess::SpawnedProcess() :
	m_hToken(NULL),
	m_hProfile(NULL),
	m_hProcess(NULL)
{
}

Root::SpawnedProcess::~SpawnedProcess(void)
{
	if (Close() == ETIMEDOUT)
		Kill();
}

int Root::SpawnedProcess::Close(ACE_Time_Value* wait)
{
	int exit_code = 0;
	DWORD dwWait = 10000;
	if (wait)
	{
		ACE_UINT64 val;
		wait->to_usec(val);
		dwWait = static_cast<DWORD>(val / 1000);
	}

	if (m_hProcess)
	{
		DWORD dwRes = WaitForSingleObject(m_hProcess,dwWait);
		if (dwRes == WAIT_TIMEOUT)
			return ETIMEDOUT;

		if (!GetExitCodeProcess(m_hProcess,&dwRes))
			return GetLastError();

		exit_code = dwRes;
		CloseHandle(m_hProcess);
		m_hProcess = NULL;
	}

	if (m_hProfile)
	{
		UnloadUserProfile(m_hToken,m_hProfile);
		m_hProfile = NULL;
	}

	if (m_hToken)
	{
		CloseHandle(m_hToken);
		m_hToken = NULL;
	}

	return exit_code;
}

void Root::SpawnedProcess::Kill()
{
	::DebugBreak();

	if (m_hProcess)
	{
		TerminateProcess(m_hProcess,UINT(-1));
		CloseHandle(m_hProcess);
		m_hProcess = NULL;
	}

	if (m_hProfile)
	{
		UnloadUserProfile(m_hToken,m_hProfile);
		m_hProfile = NULL;
	}

	if (m_hToken)
	{
		CloseHandle(m_hToken);
		m_hToken = NULL;
	}
}

bool Root::SpawnedProcess::IsRunning()
{
	if (!m_hProcess)
		return false;

	DWORD dwRes = 0;
	if (!GetExitCodeProcess(m_hProcess,&dwRes))
		return false;

	return (dwRes == STILL_ACTIVE);
}

DWORD Root::SpawnedProcess::LoadUserProfileFromToken(HANDLE hToken, HANDLE& hProfile)
{
    int err = 0;

	// Find out all about the user associated with hToken
	DWORD dwBufSize = 0;
	GetTokenInformation(hToken,TokenUser,NULL,0,&dwBufSize);
	if (dwBufSize == 0)
		return GetLastError();

	PTOKEN_USER pUserInfo = (PTOKEN_USER)new BYTE[dwBufSize];
	if (!pUserInfo)
		return ERROR_OUTOFMEMORY;

	memset(pUserInfo,0,dwBufSize);
	if (!GetTokenInformation(hToken,TokenUser,pUserInfo,dwBufSize,&dwBufSize))
	{
		delete [] pUserInfo;
		return GetLastError();
	}

	// Get the names associated with the user SID
	ACE_WString strUserName;
	ACE_WString strDomainName;

	SID_NAME_USE name_use;
	DWORD dwUNameSize = 0;
	DWORD dwDNameSize = 0;
	LookupAccountSidW(NULL,pUserInfo->User.Sid,NULL,&dwUNameSize,NULL,&dwDNameSize,&name_use);
	if (dwUNameSize == 0)
        err = GetLastError();
    else
	{
		LPWSTR pszUserName = new wchar_t[dwUNameSize];
        if (!pszUserName)
            err = GetLastError();
        else
        {
            LPWSTR pszDomainName = NULL;
            if (dwDNameSize)
            {
                pszDomainName = new wchar_t[dwDNameSize];
                if (!pszDomainName)
                    err = GetLastError();
            }

            if (err == 0)
            {
                if (!LookupAccountSidW(NULL,pUserInfo->User.Sid,pszUserName,&dwUNameSize,pszDomainName,&dwDNameSize,&name_use))
                    err = GetLastError();
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
	delete [] pUserInfo;

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
	PROFILEINFOW profile_info = {0};
    profile_info.dwSize = sizeof(PROFILEINFOW);
	profile_info.dwFlags = PI_NOUI | PI_APPLYPOLICY;
	profile_info.lpUserName = (WCHAR*)strUserName.c_str();
	profile_info.lpProfilePath = (WCHAR*)strProfilePath.c_str();
	profile_info.lpServerName = (WCHAR*)strDCName.c_str();

	BOOL bSuccess = LoadUserProfileW(hToken,&profile_info);

	if (!bSuccess)
		return GetLastError();

	hProfile = profile_info.hProfile;
	return ERROR_SUCCESS;
}

DWORD Root::SpawnedProcess::SpawnFromToken(HANDLE hToken, u_short uPort, bool bLoadProfile)
{
	// Get our module name
	TCHAR szPath[MAX_PATH];
	if (!GetModuleFileName(NULL,szPath,MAX_PATH))
		return GetLastError();

	TCHAR szCmdLine[MAX_PATH+64];
	if (ACE_OS::sprintf(szCmdLine,ACE_TEXT("\"%s\" --spawned %u"),szPath,uPort)==-1)
		return ERROR_INVALID_PARAMETER;

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
		return dwRes;
	}

	// Init our startup info
	STARTUPINFO startup_info = {0};
	startup_info.cb = sizeof(STARTUPINFO);

	DWORD dwFlags = CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT;

	PROCESS_INFORMATION process_info = {0};
	if (!CreateProcessAsUser(hToken,NULL,szCmdLine,NULL,NULL,FALSE,dwFlags,lpEnv,NULL,&startup_info,&process_info))
	{
		dwRes = GetLastError();
		DestroyEnvironmentBlock(lpEnv);
		if (hProfile)
			UnloadUserProfile(hToken,hProfile);
		return dwRes;
	}

	// Done with environment block...
	DestroyEnvironmentBlock(lpEnv);

	// Stash handles to close on end...
	m_hProfile = hProfile;
	m_hProcess = process_info.hProcess;

	// And close any others
	CloseHandle(process_info.hThread);

	return ERROR_SUCCESS;
}

bool Root::SpawnedProcess::Spawn(uid_t id, u_short uPort)
{
	HANDLE hToken = INVALID_HANDLE_VALUE;

	bool bSandbox = (id == static_cast<uid_t>(-1));
	if (bSandbox)
	{
		int err = LogonSandboxUser(&hToken);
		if (err != 0)
			return LogFailure(err);
	}
	else
	{
		// Get the process handle
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,id);
		if (!hProcess)
			return LogFailure(GetLastError());

		// Get the process token
		BOOL bSuccess = OpenProcessToken(hProcess,TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,&hToken);

		// Done with hProcess
		CloseHandle(hProcess);
		if (!bSuccess)
			return LogFailure(GetLastError());
	}

	DWORD dwRes = SpawnFromToken(hToken,uPort,!bSandbox);
	if (dwRes != ERROR_SUCCESS)
	{
		// Done with hToken
		CloseHandle(hToken);
		return LogFailure(dwRes);
	}

	m_hToken = hToken;

	return true;
}

DWORD Root::SpawnedProcess::LogonSandboxUser(HANDLE* phToken)
{
	// Get the local machine registry
	ACE_Configuration_Heap& reg_root = Manager::get_registry();

	// Get the server section
	ACE_Configuration_Section_Key sandbox_key;
	if (reg_root.open_section(reg_root.root_section(),ACE_TEXT("Server\\Sandbox"),0,sandbox_key) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Server\\Sandbox key missing from registry")),(DWORD)-1);
		
	// Get the user name and pwd...
	ACE_TString strUName;
	ACE_TString strPwd;
	if (reg_root.get_string_value(sandbox_key,ACE_TEXT("UserName"),strUName) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Sandbox username missing from registry")),(DWORD)-1);

	reg_root.get_string_value(sandbox_key,ACE_TEXT("Password"),strPwd);
	
	if (!LogonUser((TCHAR*)strUName.c_str(),NULL,(TCHAR*)strPwd.c_str(),LOGON32_LOGON_BATCH,LOGON32_PROVIDER_DEFAULT,phToken))
		return GetLastError();
	
	return ERROR_SUCCESS;
}

bool Root::SpawnedProcess::GetSandboxUid(ACE_CString& uid)
{
	HANDLE hToken;
	int err = LogonSandboxUser(&hToken);
	if (err != 0)
		return LogFailure(err);

	DWORD dwLen = 0;
	GetTokenInformation(hToken,TokenUser,NULL,0,&dwLen);
	if (dwLen == 0)
	{
		err = GetLastError();
		CloseHandle(hToken);
		return LogFailure(err);
	}

	TOKEN_USER* pBuffer = static_cast<TOKEN_USER*>(malloc(dwLen));
	if (!pBuffer)
	{
		CloseHandle(hToken);
		return LogFailure(ERROR_OUTOFMEMORY);
	}

	if (!GetTokenInformation(hToken,TokenUser,pBuffer,dwLen,&dwLen))
	{
		err = GetLastError();
		free(pBuffer);
		CloseHandle(hToken);
		return LogFailure(err);
	}

	LPSTR pszString;
	if (!ConvertSidToStringSidA(pBuffer->User.Sid,&pszString))
	{
		err = GetLastError();
		free(pBuffer);
		CloseHandle(hToken);
		return LogFailure(err);
	}

	uid = pszString;
	LocalFree(pszString);
	free(pBuffer);

	// Done with hToken
	CloseHandle(hToken);

	// Append a extra to the uid
	uid += "_SANDBOX";

	return true;
}

bool Root::SpawnedProcess::ResolveTokenToUid(uid_t token, ACE_CString& uid)
{
	// Get the process handle
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,token);
	if (!hProcess)
		return LogFailure(GetLastError());

	// Get the process token
	int err = 0;
	HANDLE hToken;
	BOOL bSuccess = OpenProcessToken(hProcess,TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,&hToken);
	
	// Done with hProcess
	CloseHandle(hProcess);
	if (!bSuccess)
		return LogFailure(err);

	DWORD dwLen = 0;
	GetTokenInformation(hToken,TokenUser,NULL,0,&dwLen);
	if (dwLen == 0)
	{
		err = GetLastError();
		CloseHandle(hToken);
		return LogFailure(err);
	}

	TOKEN_USER* pBuffer = static_cast<TOKEN_USER*>(malloc(dwLen));
	if (!pBuffer)
	{
		CloseHandle(hToken);
		return LogFailure(ERROR_OUTOFMEMORY);
	}

	if (!GetTokenInformation(hToken,TokenUser,pBuffer,dwLen,&dwLen))
	{
		err = GetLastError();
		free(pBuffer);
		CloseHandle(hToken);
		return LogFailure(err);
	}

	LPSTR pszString;
	if (!ConvertSidToStringSidA(pBuffer->User.Sid,&pszString))
	{
		err = GetLastError();
		free(pBuffer);
		CloseHandle(hToken);
		return LogFailure(err);
	}

	uid = pszString;
	LocalFree(pszString);
	free(pBuffer);

	// Done with hToken
	CloseHandle(hToken);

	return true;
}

bool Root::SpawnedProcess::CheckAccess(const char* pszFName, ACE_UINT32 mode, bool& bAllowed)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
	DWORD cbNeeded = 0;
	if (!GetFileSecurityA(pszFName,DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,pSD,0,&cbNeeded) && GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
		return false;

	pSD = static_cast<PSECURITY_DESCRIPTOR>(ACE_OS::malloc(cbNeeded));
	if (!pSD)
	{
		ACE_OS::last_error(ENOMEM);
		return false;
	}

	if (!GetFileSecurityA(pszFName,DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,pSD,cbNeeded,&cbNeeded))
	{
		ACE_OS::last_error(EINVAL);
		return false;
	}

	// Map the generic access rights
	void* TODO;	// Need to map the mode from some kind of common format...

	DWORD dwAccessDesired = static_cast<DWORD>(mode);
	GENERIC_MAPPING generic =
	{
		FILE_GENERIC_READ,
		FILE_GENERIC_WRITE,
		FILE_GENERIC_EXECUTE,
		FILE_ALL_ACCESS
	};
	MapGenericMask(&dwAccessDesired,&generic);

	// Do the access check
	PRIVILEGE_SET privilege_set = {0};

	DWORD dwPrivSetSize = sizeof(privilege_set);
	DWORD dwAccessGranted = 0;
	BOOL bAllowedVal = FALSE;
	BOOL bRes = ::AccessCheck(pSD,m_hToken,dwAccessDesired,&generic,&privilege_set,&dwPrivSetSize,&dwAccessGranted,&bAllowedVal);

	ACE_OS::free(pSD);

	if (!bRes)
	{
		ACE_OS::last_error(EINVAL);
		::DebugBreak();
		return false;
	}
	bAllowed = (bAllowedVal ? true : false);
	return true;
}

bool Root::SpawnedProcess::LogFailure(DWORD err)
{
	if (err != (DWORD)-1)
	{
		const char szDefault[] = "Unknown error code.";
		LPVOID lpMsgBuf = (LPVOID)szDefault;

		FormatMessageA( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPSTR) &lpMsgBuf,
			0,	NULL);
		
		ACE_ERROR((LM_ERROR,ACE_TEXT("%s\n"),(LPSTR)lpMsgBuf));

		// Free the buffer.
		if (lpMsgBuf != (LPVOID)szDefault)
			LocalFree(lpMsgBuf);
	}

	return false;
}

bool Root::SpawnedProcess::InstallSandbox()
{
	ACE_Configuration_Heap& reg_root = Manager::get_registry();

	// Create the server section
	ACE_Configuration_Section_Key sandbox_key;
	if (reg_root.open_section(reg_root.root_section(),ACE_TEXT("Server\\Sandbox"),1,sandbox_key)!=0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("failed to create Server\\Sandbox key in registry")),false);
	
	USER_INFO_2	info = 
	{
		L"_OMEGA_SANDBOX_USER_",   // usri2_name;
		L"_OMEGA_SANDBOX_USER_",   // usri2_password;
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
	NET_API_STATUS err = NetUserAdd(NULL,2,(LPBYTE)&info,NULL);
	if (err != NERR_Success)
		return LogFailure(err);

	// Now we have to add the SE_BATCH_LOGON_NAME priviledge and remove all the others

	// Lookup the account SID
	DWORD dwSidSize = 0;
	DWORD dwDnSize = 0;
	SID_NAME_USE use;
	LookupAccountNameW(NULL,info.usri2_name,NULL,&dwSidSize,NULL,&dwDnSize,&use);
	if (dwSidSize==0)
	{
		NetUserDel(NULL,info.usri2_name);
		err = GetLastError();
		return LogFailure(err);
	}
	
	PSID pSid = static_cast<PSID>(new char[dwSidSize]);
	if (!pSid)
	{
		NetUserDel(NULL,info.usri2_name);
		return LogFailure(ERROR_OUTOFMEMORY);
	}
	wchar_t* pszDName = new wchar_t[dwDnSize];
	if (!pszDName)
	{
		delete [] pSid;
		NetUserDel(NULL,info.usri2_name);
		return LogFailure(ERROR_OUTOFMEMORY);
	}
	if (!LookupAccountNameW(NULL,info.usri2_name,pSid,&dwSidSize,pszDName,&dwDnSize,&use))
	{
		err = GetLastError();
		delete [] pSid;
		delete [] pszDName;
		NetUserDel(NULL,info.usri2_name);
		return LogFailure(err);
	}
	delete [] pszDName;

	if (use != SidTypeUser)
	{
		delete [] pSid;
		NetUserDel(NULL,info.usri2_name);
		return LogFailure(NERR_BadUsername);
	}

	// Open the local account policy...
	LSA_HANDLE hPolicy;
	LSA_OBJECT_ATTRIBUTES oa = {0};
	NTSTATUS err2 = LsaOpenPolicy(NULL,&oa,POLICY_ALL_ACCESS,&hPolicy);
	if (err2 != 0)
	{
		delete [] pSid;
		NetUserDel(NULL,info.usri2_name);
		return LogFailure(LsaNtStatusToWinError(err2));
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
	delete [] pSid;

	// Done with policy handle
	LsaClose(hPolicy);

	if (err2 != 0)
	{
		NetUserDel(NULL,info.usri2_name);
		return LogFailure(LsaNtStatusToWinError(err2));
	}

	ACE_TString strUName(ACE_TEXT_WCHAR_TO_TCHAR(info.usri2_name));
	ACE_TString strPwd(ACE_TEXT_WCHAR_TO_TCHAR(info.usri2_password));

	// Set the user name and pwd...
	if (reg_root.set_string_value(sandbox_key,ACE_TEXT("UserName"),strUName) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("failed to set sandbox username in registry")),false);

	if (reg_root.set_string_value(sandbox_key,ACE_TEXT("Password"),strPwd) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("failed to set sandbox password in registry")),false);
	
	return true;
}

bool Root::SpawnedProcess::UninstallSandbox()
{
	ACE_Configuration_Heap& reg_root = Manager::get_registry();

	// Open the server section
	ACE_Configuration_Section_Key sandbox_key;
	if (reg_root.open_section(reg_root.root_section(),ACE_TEXT("Server\\Sandbox"),0,sandbox_key)!=0)
		return true;
	
	ACE_TString strUName;
	
	// Set the user name and pwd...
	if (reg_root.get_string_value(sandbox_key,ACE_TEXT("UserName"),strUName) != 0)
		return true;

	NetUserDel(NULL,ACE_TEXT_ALWAYS_WCHAR(strUName.c_str()));

	return true;
}

#endif // ACE_WIN32
