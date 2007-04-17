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
	// Impersonate the logged on user
	if (!ImpersonateLoggedOnUser(hToken))
		return GetLastError();

	DWORD dwProfileFlags = 0;
	DWORD dwRes = ERROR_SUCCESS;
	if (!GetProfileType(&dwProfileFlags))
		dwRes = GetLastError();

	// If we can't revert we're screwed!
	if (!RevertToSelf())
		abort();

	if (dwRes != ERROR_SUCCESS && dwRes != ERROR_FILE_NOT_FOUND)
		return dwRes;

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
	SID_NAME_USE name_use;
	DWORD dwUNameSize = 0;
	DWORD dwDNameSize = 0;
	LookupAccountSidW(NULL,pUserInfo->User.Sid,NULL,&dwUNameSize,NULL,&dwDNameSize,&name_use);

	if (dwUNameSize == 0)
	{
		delete [] pUserInfo;
		return GetLastError();
	}
	LPWSTR pszUserName = new wchar_t[dwUNameSize];
	if (!pszUserName)
	{
		delete [] pUserInfo;
		return GetLastError();
	}

	LPWSTR pszDomainName = NULL;
	if (dwDNameSize)
	{
		pszDomainName = new wchar_t[dwDNameSize];
		if (!pszDomainName)
		{
			delete [] pszUserName;
			delete [] pUserInfo;
			return GetLastError();
		}
	}
	if (!LookupAccountSidW(NULL,pUserInfo->User.Sid,pszUserName,&dwUNameSize,pszDomainName,&dwDNameSize,&name_use))
	{
		delete [] pszDomainName;
		delete [] pszUserName;
		delete [] pUserInfo;
		return GetLastError();
	}

	// Done with user info
	delete [] pUserInfo;

	LPWSTR pszUserProfilePath = NULL;
	LPWSTR pszDCName = NULL;

	// If the user has a roaming profile
	if (dwProfileFlags & PT_ROAMING)
	{
		// Lookup a DC for pszDomain
		LPWSTR pszDCName2 = NULL;
		if (NetGetAnyDCName(NULL,pszDomainName,(LPBYTE*)&pszDCName2) == NERR_Success)
		{
			pszDCName = new wchar_t[wcslen(pszDCName2)+1];
			if (!pszDCName)
			{
				delete [] pszDomainName;
				delete [] pszUserName;
				return ERROR_OUTOFMEMORY;
			}
			wcscpy(pszDCName,pszDCName2);

			// Try to find the user's roaming profile...
			USER_INFO_3* pInfo = NULL;
			if (NetUserGetInfo(pszDCName,pszUserName,3,(LPBYTE*)&pInfo) == NERR_Success)
			{
				if (pInfo->usri3_profile)
				{
					// Alloc and copy the profile path
					pszUserProfilePath = new wchar_t[wcslen(pInfo->usri3_profile)+1];
					if (!pszUserProfilePath)
					{
						delete [] pszDCName;
						delete [] pszDomainName;
						delete [] pszUserName;
						return ERROR_OUTOFMEMORY;
					}
					wcscpy(pszUserProfilePath,pInfo->usri3_profile);
				}
				NetApiBufferFree(pInfo);
			}
			NetApiBufferFree(pszDCName2);
		}
	}

	// Done with domain name
	delete [] pszDomainName;

	// Load the Users Profile
	PROFILEINFOW profile_info = {0};
    profile_info.dwSize = sizeof(PROFILEINFOW);
	profile_info.dwFlags = PI_NOUI | PI_APPLYPOLICY;
	profile_info.lpUserName = pszUserName;
	profile_info.lpProfilePath = pszUserProfilePath;
	profile_info.lpServerName = pszDCName;

	BOOL bSuccess = LoadUserProfileW(hToken,&profile_info);

	// Done with the strings now...
	delete [] pszDCName;
	delete [] pszUserName;
	delete [] pszUserProfilePath;

	if (!bSuccess)
		return GetLastError();
	
	hProfile = profile_info.hProfile;
	return ERROR_SUCCESS;    
}

DWORD Root::SpawnedProcess::SpawnFromToken(HANDLE hToken, u_short uPort)
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
	DWORD dwRes = LoadUserProfileFromToken(hToken,hProfile);
	if (dwRes != ERROR_SUCCESS)
		return dwRes;
	
	// Load the Users Environment vars
	LPVOID lpEnv = NULL;
	if (!CreateEnvironmentBlock(&lpEnv,hToken,FALSE))
	{
		dwRes = GetLastError();
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

int Root::SpawnedProcess::Spawn(Session::TOKEN id, u_short uPort)
{
	HANDLE hToken = INVALID_HANDLE_VALUE;

	if (id == static_cast<Session::TOKEN>(-1))
	{
		int err = LogonSandboxUser(&hToken);
		if (err != 0)
			return err;
	}
	else
	{
		// Get the process handle
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,id);
		if (!hProcess)
			return GetLastError();

		// Get the process token
		BOOL bSuccess = OpenProcessToken(hProcess,TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,&hToken);

		// Done with hProcess
		CloseHandle(hProcess);
		if (!bSuccess)
			return GetLastError();
	}

	DWORD dwRes = SpawnFromToken(hToken,uPort);
	if (dwRes != ERROR_SUCCESS)
	{
		// Done with hToken
		CloseHandle(hToken);
		return dwRes;
	}
	
	m_hToken = hToken;

	return 0;
}

int Root::SpawnedProcess::LogonSandboxUser(HANDLE* phToken)
{
	// Get the local machine registry
	ACE_Configuration_Heap& reg_root = Manager::get_registry();

	// Get the server section
	ACE_Configuration_Section_Key sandbox_key;
	int ret = reg_root.open_section(reg_root.root_section(),ACE_TEXT("Server\\Sandbox"),0,sandbox_key);
	if (ret != 0)
		return ret;

	// Get the user name and pwd...
	ACE_TString strUName;
	ret = reg_root.get_string_value(sandbox_key,ACE_TEXT("UserName"),strUName);
	if (ret != 0)
		//return ret;
		strUName = ACE_TEXT("OMEGA_SANDBOX");
		
	ACE_TString strPwd;
	reg_root.get_string_value(sandbox_key,ACE_TEXT("Password"),strPwd);
	
	if (!LogonUser(strUName.c_str(),NULL,strPwd.c_str(),LOGON32_LOGON_BATCH,LOGON32_PROVIDER_DEFAULT,phToken))
		return GetLastError();

	return 0;
}

int Root::SpawnedProcess::GetSandboxUid(ACE_CString& uid)
{
	HANDLE hToken;
	int err = LogonSandboxUser(&hToken);
	if (err != 0)
		return err;		

	DWORD dwLen = 0;
	GetTokenInformation(hToken,TokenUser,NULL,0,&dwLen);
	if (dwLen == 0)
	{
		err = GetLastError();
		CloseHandle(hToken);
		return err;
	}
	
	TOKEN_USER* pBuffer = static_cast<TOKEN_USER*>(malloc(dwLen));
	if (!pBuffer)
	{
		CloseHandle(hToken);
		return ERROR_OUTOFMEMORY;
	}

	if (!GetTokenInformation(hToken,TokenUser,pBuffer,dwLen,&dwLen))
	{
		err = GetLastError();
		free(pBuffer);
		CloseHandle(hToken);
		return err;
	}

	LPSTR pszString;
	if (!ConvertSidToStringSidA(pBuffer->User.Sid,&pszString))
	{
		err = GetLastError();
		free(pBuffer);
		CloseHandle(hToken);
		return err;
	}

	uid = pszString;
	LocalFree(pszString);
	free(pBuffer);

	// Done with hToken
	CloseHandle(hToken);

	// Append a extra to the uid
	uid += "_SANDBOX";

	return 0;
}

int Root::SpawnedProcess::ResolveTokenToUid(Session::TOKEN token, ACE_CString& uid)
{
	// Get the process handle
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,token);
	if (!hProcess)
		return GetLastError();

	// Get the process token
	int err = 0;
	HANDLE hToken;
	BOOL bSuccess = OpenProcessToken(hProcess,TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,&hToken);
	if (!bSuccess)
		err = GetLastError();

	// Done with hProcess
	CloseHandle(hProcess);
	if (!bSuccess)
		return err;

	DWORD dwLen = 0;
	GetTokenInformation(hToken,TokenUser,NULL,0,&dwLen);
	if (dwLen == 0)
	{
		err = GetLastError();
		CloseHandle(hToken);
		return err;
	}
	
	TOKEN_USER* pBuffer = static_cast<TOKEN_USER*>(malloc(dwLen));
	if (!pBuffer)
	{
		CloseHandle(hToken);
		return ERROR_OUTOFMEMORY;
	}

	if (!GetTokenInformation(hToken,TokenUser,pBuffer,dwLen,&dwLen))
	{
		err = GetLastError();
		free(pBuffer);
		CloseHandle(hToken);
		return err;
	}

	LPSTR pszString;
	if (!ConvertSidToStringSidA(pBuffer->User.Sid,&pszString))
	{
		err = GetLastError();
		free(pBuffer);
		CloseHandle(hToken);
		return err;
	}

	uid = pszString;
	LocalFree(pszString);
	free(pBuffer);

	// Done with hToken
	CloseHandle(hToken);

	return 0;
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
		return false;

	// Map the generic access rights

	void* TODO;	// Need to map mode the mode from some kind of common format...

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
		::DebugBreak();
		return false;
	}
	bAllowed = (bAllowedVal ? true : false);
	return true;
}

#endif // ACE_WIN32
