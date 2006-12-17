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

#include "./SpawnedProcess.h"

#ifdef ACE_WIN32

#include <ace/OS.h>

#include <userenv.h>
#include <lm.h>
#include <sddl.h>

SpawnedProcess::SpawnedProcess() :
	m_hToken(NULL),
	m_hProfile(NULL),
	m_hProcess(NULL)
{
}

SpawnedProcess::~SpawnedProcess(void)
{
	Close();	
}

int SpawnedProcess::Close(ACE_Time_Value* wait)
{
	int exit_code = 0;
	DWORD dwWait = INFINITE;
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

bool SpawnedProcess::IsRunning()
{
	if (!m_hProcess)
		return false;

	DWORD dwRes = 0;
	if (!GetExitCodeProcess(m_hProcess,&dwRes))
		return false;

	return (dwRes == STILL_ACTIVE);
}

DWORD SpawnedProcess::LoadUserProfileFromToken(HANDLE hToken, HANDLE& hProfile)
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

	if (dwRes != ERROR_SUCCESS)
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

DWORD SpawnedProcess::SpawnFromToken(HANDLE hToken, u_short uPort)
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
	
#ifdef _DEBUG
	startup_info.lpDesktop = ACE_TEXT("winsta0\\default");
	DWORD dwFlags = CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;
#else
	DWORD dwFlags = CREATE_NO_WINDOW | DETACHED_PROCESS | CREATE_UNICODE_ENVIRONMENT;
#endif

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

int SpawnedProcess::Spawn(Session::TOKEN id, u_short uPort)
{
	// Get the process handle
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,id);
	if (!hProcess)
		return GetLastError();

	// Get the process token
	HANDLE hToken;
	BOOL bSuccess = OpenProcessToken(hProcess,TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,&hToken);

	// Done with hProcess
	CloseHandle(hProcess);
	if (!bSuccess)
		return GetLastError();

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

int SpawnedProcess::ResolveTokenToUid(Session::TOKEN token, ACE_CString& uid)
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

#endif // ACE_WIN32
