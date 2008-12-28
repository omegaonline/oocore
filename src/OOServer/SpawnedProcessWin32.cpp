///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
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
#include "./SpawnedProcess.h"

#if defined(OMEGA_WIN32)

#include <userenv.h>
#include <lm.h>
#include <sddl.h>
#include <ntsecapi.h>
#include <aclapi.h>
#include <shlwapi.h>

#if defined(__MINGW32__)

typedef struct _TOKEN_GROUPS_AND_PRIVILEGES {
    DWORD SidCount;
    DWORD SidLength;
    PSID_AND_ATTRIBUTES Sids;
    DWORD RestrictedSidCount;
    DWORD RestrictedSidLength;
    PSID_AND_ATTRIBUTES RestrictedSids;
    DWORD PrivilegeCount;
    DWORD PrivilegeLength;
    PLUID_AND_ATTRIBUTES Privileges;
    LUID AuthenticationId;
} TOKEN_GROUPS_AND_PRIVILEGES;

extern "C"
WINADVAPI
BOOL
APIENTRY
CreateRestrictedToken(
    HANDLE ExistingTokenHandle,
    DWORD Flags,
    DWORD DisableSidCount,
    PSID_AND_ATTRIBUTES SidsToDisable,
    DWORD DeletePrivilegeCount,
    PLUID_AND_ATTRIBUTES PrivilegesToDelete,
    DWORD RestrictedSidCount,
    PSID_AND_ATTRIBUTES SidsToRestrict,
    PHANDLE NewTokenHandle
    );

#define DISABLE_MAX_PRIVILEGE   0x1
#define SANDBOX_INERT           0x2

#else
// Not available under MinGW
#include <WinSafer.h>
#endif

#if !defined(OMEGA_WIDEN_STRING)
#define OMEGA_WIDEN_STRING_II(STRING) L ## STRING
#define OMEGA_WIDEN_STRING_I(STRING)  OMEGA_WIDEN_STRING_II(STRING)
#define OMEGA_WIDEN_STRING(STRING)    OMEGA_WIDEN_STRING_I(STRING)
#endif

#define LOG_FAILURE(err) LogFailure((err),OMEGA_WIDEN_STRING(__FILE__),__LINE__)

#ifndef PROTECTED_DACL_SECURITY_INFORMATION
#define PROTECTED_DACL_SECURITY_INFORMATION	 (0x80000000L)
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
		if (m_hProfile)
		{
			// We only need to wait if we have loaded the profile...
			DWORD dwWait = 25000;
			DWORD dwRes = WaitForSingleObject(m_hProcess,dwWait);
			if (dwRes != WAIT_OBJECT_0)
				TerminateProcess(m_hProcess,UINT(-1));
		}

		CloseHandle(m_hProcess);
	}

	if (m_hProfile)
		UnloadUserProfile(m_hToken,m_hProfile);

	if (m_hToken)
		CloseHandle(m_hToken);
}

DWORD Root::SpawnedProcess::GetNameFromToken(HANDLE hToken, ACE_WString& strUserName, ACE_WString& strDomainName)
{
	DWORD err = 0;

	// Find out all about the user associated with hToken
	TOKEN_USER* pUserInfo = static_cast<TOKEN_USER*>(GetTokenInfo(hToken,TokenUser));
	if (!pUserInfo)
	{
		err = GetLastError();
		LOG_FAILURE(err);
		return err;
	}

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

	return err;
}

DWORD Root::SpawnedProcess::LoadUserProfileFromToken(HANDLE hToken, HANDLE& hProfile)
{
	// Get the names associated with the user SID
	ACE_WString strUserName;
	ACE_WString strDomainName;

	DWORD err = GetNameFromToken(hToken,strUserName,strDomainName);
	if (err != ERROR_SUCCESS)
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

DWORD Root::SpawnedProcess::GetLogonSID(HANDLE hToken, PSID& pSIDLogon)
{
	// Get the logon SID of the Token
	TOKEN_GROUPS* pGroups = static_cast<TOKEN_GROUPS*>(GetTokenInfo(hToken,TokenGroups));
	if (!pGroups)
		return GetLastError();

	DWORD dwRes = ERROR_INVALID_SID;

	// Loop through the groups to find the logon SID
	for (DWORD dwIndex = 0; dwIndex < pGroups->GroupCount; dwIndex++)
	{
		if ((pGroups->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID) ==  SE_GROUP_LOGON_ID)
		{
			// Found the logon SID...
			DWORD dwLen = GetLengthSid(pGroups->Groups[dwIndex].Sid);
			pSIDLogon = static_cast<PSID>(ACE_OS::malloc(dwLen));
			if (!pSIDLogon)
				dwRes = ERROR_OUTOFMEMORY;
			else
			{
				if (!CopySid(dwLen,pSIDLogon,pGroups->Groups[dwIndex].Sid))
					dwRes = GetLastError();
				else
					dwRes = ERROR_SUCCESS;
			}
			break;
		}
	}

	ACE_OS::free(pGroups);

	return dwRes;
}

DWORD Root::SpawnedProcess::CreateSD(PACL pACL, void*& pSD)
{
	// Create a new security descriptor
	pSD = LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (pSD == NULL)
		return GetLastError();

	DWORD dwRes = ERROR_SUCCESS;

	// Initialize a security descriptor.
	if (!InitializeSecurityDescriptor(static_cast<PSECURITY_DESCRIPTOR>(pSD),SECURITY_DESCRIPTOR_REVISION))
	{
		dwRes = GetLastError();
		LocalFree(pSD);
		return dwRes;
	}

	// Add the ACL to the SD
	if (!SetSecurityDescriptorDacl(static_cast<PSECURITY_DESCRIPTOR>(pSD),TRUE,pACL,FALSE))
	{
		dwRes = GetLastError();
		LocalFree(pSD);
		return dwRes;
	}

	return ERROR_SUCCESS;
}

DWORD Root::SpawnedProcess::CreateWindowStationSD(TOKEN_USER* pProcessUser, PSID pSIDLogon, PACL& pACL, void*& pSD)
{
	const int NUM_ACES = 3;
	EXPLICIT_ACCESSW ea[NUM_ACES];
	ZeroMemory(&ea, NUM_ACES * sizeof(EXPLICIT_ACCESS));

	// Set minimum access for the calling process SID
	ea[0].grfAccessPermissions = WINSTA_CREATEDESKTOP;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[0].Trustee.ptstrName = (LPWSTR)pProcessUser->User.Sid;

	// Set default access for Specific logon.
	ea[1].grfAccessPermissions =
		WINSTA_ACCESSCLIPBOARD |
		WINSTA_ACCESSGLOBALATOMS |
		WINSTA_CREATEDESKTOP |
		WINSTA_EXITWINDOWS |
		WINSTA_READATTRIBUTES |
		STANDARD_RIGHTS_REQUIRED;

	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[1].Trustee.ptstrName = (LPWSTR)pSIDLogon;

	// Set generic all access for Specific logon for everything below...
	ea[2].grfAccessPermissions = STANDARD_RIGHTS_REQUIRED | GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL;
	ea[2].grfAccessMode = SET_ACCESS;
	ea[2].grfInheritance = CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
	ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[2].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[2].Trustee.ptstrName = (LPWSTR)pSIDLogon;

	DWORD dwRes = SetEntriesInAclW(NUM_ACES,ea,NULL,&pACL);
	if (dwRes != ERROR_SUCCESS)
		return dwRes;

	dwRes = CreateSD(pACL,pSD);
	if (dwRes != ERROR_SUCCESS)
		LocalFree(pACL);

	return dwRes;
}

DWORD Root::SpawnedProcess::CreateDesktopSD(TOKEN_USER* pProcessUser, PSID pSIDLogon, PACL& pACL, void*& pSD)
{
	const int NUM_ACES = 2;
	EXPLICIT_ACCESSW ea[NUM_ACES];
	ZeroMemory(&ea, NUM_ACES * sizeof(EXPLICIT_ACCESS));

	// Set minimum access for the calling process SID
	ea[0].grfAccessPermissions = DESKTOP_CREATEWINDOW;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[0].Trustee.ptstrName = (LPWSTR)pProcessUser->User.Sid;

	// Set full access for Specific logon.
	ea[1].grfAccessPermissions =
		DESKTOP_CREATEMENU |
		DESKTOP_CREATEWINDOW |
		DESKTOP_ENUMERATE |
		DESKTOP_HOOKCONTROL |
		DESKTOP_READOBJECTS |
		DESKTOP_WRITEOBJECTS |
		STANDARD_RIGHTS_REQUIRED;

	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[1].Trustee.ptstrName = (LPWSTR)pSIDLogon;

	DWORD dwRes = SetEntriesInAclW(NUM_ACES,ea,NULL,&pACL);
	if (dwRes != ERROR_SUCCESS)
		return dwRes;

	dwRes = CreateSD(pACL,pSD);
	if (dwRes != ERROR_SUCCESS)
		LocalFree(pACL);

	return dwRes;
}

DWORD Root::SpawnedProcess::OpenCorrectWindowStation(HANDLE hToken, ACE_WString& strWindowStation, HWINSTA& hWinsta, HDESK& hDesktop)
{
	// Service window stations are created with the name "Service-0xZ1-Z2$",
	// where Z1 is the high part of the logon SID and Z2 is the low part of the logon SID
	// see http://msdn2.microsoft.com/en-us/library/ms687105.aspx for details

	// Get the logon SID of the Token
	PSID pSIDLogon = 0;
	DWORD dwRes = GetLogonSID(hToken,pSIDLogon);
	if (dwRes != ERROR_SUCCESS)
		return dwRes;

	LPWSTR pszSid = 0;
	if (!ConvertSidToStringSidW(pSIDLogon,&pszSid))
	{
		dwRes = GetLastError();
		ACE_OS::free(pSIDLogon);
		return dwRes;
	}
	strWindowStation = pszSid;
	LocalFree(pszSid);

	// Logon SIDs are of the form S-1-5-5-X-Y, and we want X and Y
	if (ACE_OS::strncmp(strWindowStation.c_str(),L"S-1-5-5-",8) != 0)
	{
		ACE_OS::free(pSIDLogon);
		return ERROR_INVALID_SID;
	}

	// Crack out the last two parts - there is probably an easier way... but this works
	strWindowStation = strWindowStation.substr(8);
	const wchar_t* p = strWindowStation.c_str();
	wchar_t* pEnd = 0;
	DWORD dwParts[2];
	dwParts[0] = ACE_OS::strtoul(p,&pEnd,10);
	if (*pEnd != L'-')
	{
		ACE_OS::free(pSIDLogon);
		return ERROR_INVALID_SID;
	}
	dwParts[1] = ACE_OS::strtoul(pEnd+1,&pEnd,10);
	if (*pEnd != L'\0')
	{
		ACE_OS::free(pSIDLogon);
		return ERROR_INVALID_SID;
	}

	wchar_t szBuf[256];
	ACE_OS::snprintf(szBuf,255,L"Service-0x%lx-%lx$",dwParts[0],dwParts[1]);
	strWindowStation = szBuf;

	// Get the current processes user SID
	HANDLE hProcessToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
	{
		dwRes = GetLastError();
		ACE_OS::free(pSIDLogon);
		return dwRes;
	}

	TOKEN_USER* pProcessUser = static_cast<TOKEN_USER*>(GetTokenInfo(hProcessToken,TokenUser));
	if (!pProcessUser)
	{
		dwRes = GetLastError();
		ACE_OS::free(pSIDLogon);
		CloseHandle(hProcessToken);
		return dwRes;
	}

	CloseHandle(hProcessToken);

	// Open or create the new window station and desktop
	// Now confirm the Window Station exists...
	hWinsta = OpenWindowStationW((LPWSTR)strWindowStation.c_str(),FALSE,WINSTA_CREATEDESKTOP);
	if (!hWinsta)
	{
		// See if just doesn't exist yet...
		dwRes = GetLastError();
		if (dwRes != ERROR_FILE_NOT_FOUND)
		{
			ACE_OS::free(pSIDLogon);
			ACE_OS::free(pProcessUser);
			return dwRes;
		}

		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = FALSE;
		sa.lpSecurityDescriptor = 0;
		PACL pACL = 0;

		dwRes = CreateWindowStationSD(pProcessUser,pSIDLogon,pACL,sa.lpSecurityDescriptor);
		if (dwRes != ERROR_SUCCESS)
		{
			ACE_OS::free(pSIDLogon);
			ACE_OS::free(pProcessUser);
			return dwRes;
		}

		hWinsta = CreateWindowStationW(strWindowStation.c_str(),0,WINSTA_CREATEDESKTOP,&sa);
		if (!hWinsta)
			dwRes = GetLastError();

		LocalFree(pACL);
		LocalFree(sa.lpSecurityDescriptor);

		if (!hWinsta)
		{
			ACE_OS::free(pSIDLogon);
			ACE_OS::free(pProcessUser);
			return GetLastError();
		}
	}

	// Stash our old
	HWINSTA hOldWinsta = GetProcessWindowStation();
	if (!hOldWinsta)
	{
		dwRes = GetLastError();
		ACE_OS::free(pSIDLogon);
		ACE_OS::free(pProcessUser);
		CloseHandle(hWinsta);
		return dwRes;
	}

	// Swap our Window Station
	if (!SetProcessWindowStation(hWinsta))
	{
		dwRes = GetLastError();
		ACE_OS::free(pSIDLogon);
		ACE_OS::free(pProcessUser);
		CloseHandle(hWinsta);
		return dwRes;
	}

	// Try for the desktop
	hDesktop = OpenDesktopW(L"default",0,FALSE,DESKTOP_CREATEWINDOW);
	if (!hDesktop)
	{
		// See if just doesn't exist yet...
		dwRes = GetLastError();
		if (dwRes != ERROR_FILE_NOT_FOUND)
		{
			ACE_OS::free(pSIDLogon);
			ACE_OS::free(pProcessUser);
			SetProcessWindowStation(hOldWinsta);
			CloseHandle(hWinsta);
			return dwRes;
		}

		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = FALSE;
		sa.lpSecurityDescriptor = 0;
		PACL pACL = 0;

		dwRes = CreateDesktopSD(pProcessUser,pSIDLogon,pACL,sa.lpSecurityDescriptor);
		if (dwRes != ERROR_SUCCESS)
		{
			ACE_OS::free(pSIDLogon);
			ACE_OS::free(pProcessUser);
			return dwRes;
		}

		hDesktop = CreateDesktopW(L"default",0,0,0,DESKTOP_CREATEWINDOW,&sa);
		if (!hDesktop)
			dwRes = GetLastError();

		LocalFree(pACL);
		LocalFree(sa.lpSecurityDescriptor);

		if (!hDesktop)
		{
			ACE_OS::free(pSIDLogon);
			ACE_OS::free(pProcessUser);
			SetProcessWindowStation(hOldWinsta);
			CloseHandle(hWinsta);
			return dwRes;
		}
	}

	// Revert our Window Station
	SetProcessWindowStation(hOldWinsta);

	ACE_OS::free(pSIDLogon);
	ACE_OS::free(pProcessUser);

	strWindowStation += L"\\default";

	return dwRes;
}

DWORD Root::SpawnedProcess::SetTokenDefaultDACL(HANDLE hToken)
{
	// Get the current Default DACL
	TOKEN_DEFAULT_DACL* pDef_dacl = static_cast<TOKEN_DEFAULT_DACL*>(GetTokenInfo(hToken,TokenDefaultDacl));
	if (!pDef_dacl)
		return ERROR_OUTOFMEMORY;

	// Get the logon SID of the Token
	PSID pSIDLogon = 0;
	DWORD dwRes = GetLogonSID(hToken,pSIDLogon);
	if (dwRes != ERROR_SUCCESS)
	{
		ACE_OS::free(pDef_dacl);
		return dwRes;
	}

	const int NUM_ACES = 1;
	EXPLICIT_ACCESSW ea[NUM_ACES];
	ZeroMemory(&ea, NUM_ACES * sizeof(EXPLICIT_ACCESS));

	// Set maximum access for the logon SID
	ea[0].grfAccessPermissions = GENERIC_ALL;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[0].Trustee.ptstrName = (LPWSTR)pSIDLogon;

	TOKEN_DEFAULT_DACL def_dacl = {0};
	dwRes = SetEntriesInAclW(NUM_ACES,ea,pDef_dacl->DefaultDacl,&def_dacl.DefaultDacl);

	ACE_OS::free(pSIDLogon);
	ACE_OS::free(pDef_dacl);

	if (dwRes != ERROR_SUCCESS)
		return dwRes;

	// Now set the token default DACL
	if (!SetTokenInformation(hToken,TokenDefaultDacl,&def_dacl,sizeof(def_dacl)))
	{
		dwRes = GetLastError();
		LocalFree(def_dacl.DefaultDacl);
	}

	return dwRes;
}

void Root::SpawnedProcess::EnableUserAccessToFile(LPWSTR pszPath, TOKEN_USER* pUser)
{
	PathRemoveFileSpecW(pszPath);

	PACL pACL = 0;
	PSECURITY_DESCRIPTOR pSD = 0;
	DWORD dwRes = GetNamedSecurityInfoW(pszPath,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,&pACL,NULL,&pSD);
	if (dwRes != ERROR_SUCCESS)
		return;

	const int NUM_ACES = 1;
	EXPLICIT_ACCESSW ea[NUM_ACES];
	ZeroMemory(&ea, NUM_ACES * sizeof(EXPLICIT_ACCESS));

	// Set maximum access for the logon SID
	ea[0].grfAccessPermissions = GENERIC_ALL;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = OBJECT_INHERIT_ACE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[0].Trustee.ptstrName = (LPWSTR)pUser->User.Sid;

	PACL pACLNew = {0};
	dwRes = SetEntriesInAclW(NUM_ACES,ea,pACL,&pACLNew);

	LocalFree(pSD);

	if (dwRes != ERROR_SUCCESS)
		return;

	dwRes = SetNamedSecurityInfoW(pszPath,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,NULL,NULL,pACLNew,NULL);
}

bool Root::SpawnedProcess::RestrictToken(HANDLE& hToken)
{
	// Work out what version of windows we are running on...
	OSVERSIONINFO os = {0};
	os.dwOSVersionInfoSize = sizeof(os);
	GetVersionEx(&os);

#if !defined(OMEGA_DEBUG)
	if ((os.dwMajorVersion == 5 && os.dwMinorVersion > 0) || os.dwMajorVersion >= 5)
	{
		//// Use SAFER API
		//SAFER_LEVEL_HANDLE hAuthzLevel = NULL;
		//if (!SaferCreateLevel(SAFER_SCOPEID_MACHINE,SAFER_LEVELID_UNTRUSTED,SAFER_LEVEL_OPEN,&hAuthzLevel,NULL))
		//	return false;

		//// Generate the restricted token we will use.
		//bool bOk = false;
		//HANDLE hNewToken = NULL;
		//if (SaferComputeTokenFromLevel(
		//	hAuthzLevel,    // SAFER Level handle
		//	hToken,         // Source token
		//	&hNewToken,     // Target token
		//	0,              // No flags
		//	NULL))          // Reserved
		//{
		//	// Swap the tokens
		//	CloseHandle(hToken);
		//	hToken = hNewToken;
		//	bOk = true;
		//}

		//SaferCloseLevel(hAuthzLevel);
		//
		//if (!bOk)
		//	return false;
	}
#endif

	if (os.dwMajorVersion >= 5)
	{
		// Create a restricted token...
		HANDLE hNewToken = NULL;
		if (!CreateRestrictedToken(hToken,DISABLE_MAX_PRIVILEGE | SANDBOX_INERT,0,NULL,0,NULL,0,NULL,&hNewToken))
			return false;

		CloseHandle(hToken);
		hToken = hNewToken;
	}

	if (os.dwMajorVersion > 5)
	{
		// Vista - use UAC as well...
	}

	return true;
}

DWORD Root::SpawnedProcess::SpawnFromToken(HANDLE hToken, const ACE_CString& strPipe, bool bSandbox)
{
	// Get our module name
	wchar_t szPath[MAX_PATH];
	if (!GetModuleFileNameW(NULL,szPath,MAX_PATH))
	{
		DWORD dwErr = GetLastError();
		LOG_FAILURE(dwErr);
		return dwErr;
	}

	// Strip off our name, and add OOSvrUser.exe
	PathRemoveFileSpecW(szPath);

	wchar_t szCwd[MAX_PATH];
	ACE_OS::strncpy(szCwd,szPath,MAX_PATH-1);

	PathAppendW(szPath,L"OOSvrUser.exe");

	ACE_WString strCmdLine = L"\"";
	strCmdLine += szPath;
	strCmdLine += L"\" ";
	strCmdLine += ACE_Ascii_To_Wide(strPipe.c_str()).wchar_rep();

	// Forward declare these because of goto's
	ACE_WString strWindowStation;
	HWINSTA hWinsta = 0;
	HDESK hDesktop = 0;
	DWORD dwFlags = CREATE_UNICODE_ENVIRONMENT | CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_PROCESS_GROUP;
	HANDLE hDebugEvent = NULL;
	HANDLE hPriToken = 0;
	ACE_WString strTitle;

	// Load up the users profile
	HANDLE hProfile = NULL;
	DWORD dwRes = 0;
	if (!bSandbox)
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
		goto Cleanup;
	}

	// Get the primary token from the impersonation token
	if (!DuplicateTokenEx(hToken,TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_DEFAULT,NULL,SecurityImpersonation,TokenPrimary,&hPriToken))
	{
		dwRes = GetLastError();
		goto Cleanup;
	}

	// Init our startup info
	STARTUPINFOW startup_info;
	ACE_OS::memset(&startup_info,0,sizeof(startup_info));
	startup_info.cb = sizeof(STARTUPINFOW);
	startup_info.dwFlags = STARTF_USESHOWWINDOW;
	startup_info.wShowWindow = SW_MINIMIZE;

#if defined(OMEGA_DEBUG)
	if (IsDebuggerPresent())
	{
		hDebugEvent = CreateEventW(NULL,FALSE,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX");
		if (!hDebugEvent && GetLastError()==ERROR_ALREADY_EXISTS)
			hDebugEvent = OpenEventW(EVENT_ALL_ACCESS,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX");

		dwFlags |= CREATE_NEW_CONSOLE;

		strTitle = szPath;

		// Get the names associated with the user SID
        ACE_WString strUserName;
        ACE_WString strDomainName;
        if (GetNameFromToken(hPriToken,strUserName,strDomainName) == ERROR_SUCCESS)
        {
            strTitle += L" - ";
            strTitle += strDomainName;
            strTitle += L"\\";
            strTitle += strUserName;
        }

        if (bSandbox)
            strTitle += L" [Sandbox]";
	}
	else
#endif
	{
		dwFlags |= DETACHED_PROCESS;

		if (bSandbox)
		{
			dwRes = OpenCorrectWindowStation(hPriToken,strWindowStation,hWinsta,hDesktop);
			if (dwRes != ERROR_SUCCESS)
				goto Cleanup;

			startup_info.lpDesktop = const_cast<LPWSTR>(strWindowStation.c_str());
		}
		else
		{
			startup_info.lpDesktop = L"";
			startup_info.wShowWindow = SW_HIDE;
		}
	}
	if (!strTitle.empty())
        startup_info.lpTitle = (LPWSTR)strTitle.c_str();

	// Actually create the process!
	PROCESS_INFORMATION process_info;
	if (!CreateProcessAsUserW(hPriToken,NULL,(wchar_t*)strCmdLine.c_str(),NULL,NULL,FALSE,dwFlags,lpEnv,szCwd,&startup_info,&process_info))
	{
		dwRes = GetLastError();

		if (hDebugEvent)
			CloseHandle(hDebugEvent);

		goto Cleanup;
	}

	// Attach a debugger if we are debugging
#if defined(OMEGA_DEBUG)
	if (hDebugEvent)
	{
		AttachDebugger(process_info.dwProcessId);
		if (hDebugEvent)
		{
			SetEvent(hDebugEvent);
			CloseHandle(hDebugEvent);
		}
	}
#endif

	// See if process has immediately terminated...
	DWORD dwWait = WaitForSingleObject(process_info.hProcess,100);
	if (dwWait != WAIT_TIMEOUT)
	{
		// Process aborted very quickly... this can happen if we have security issues
		GetExitCodeProcess(process_info.hProcess,&dwRes);
		if (dwRes == ERROR_SUCCESS)
			dwRes = ERROR_INTERNAL_ERROR;

		CloseHandle(process_info.hProcess);
		CloseHandle(process_info.hThread);

		goto Cleanup;
	}

	// Stash handles to close on end...
	m_hProfile = hProfile;
	m_hProcess = process_info.hProcess;

	// Duplicate the impersonated token...
	DuplicateToken(hToken,SecurityImpersonation,&m_hToken);

	// And close any others
	CloseHandle(process_info.hThread);

	// Don't want to close this one...
	hProfile = NULL;

Cleanup:
	// Done with hPriToken
	if (hPriToken)
		CloseHandle(hPriToken);

	// Done with Desktop
	if (hDesktop)
		CloseDesktop(hDesktop);

	// Done with Window Station
	if (hWinsta)
		CloseWindowStation(hWinsta);

	// Done with environment block...
	if (lpEnv)
		DestroyEnvironmentBlock(lpEnv);

	if (hProfile)
		UnloadUserProfile(hToken,hProfile);

	if (dwRes != ERROR_SUCCESS)
		LOG_FAILURE(dwRes);

	return dwRes;
}

bool Root::SpawnedProcess::Spawn(user_id_type hToken, const ACE_CString& strPipe, bool bSandbox)
{
	m_bSandbox = bSandbox;

	DWORD dwRes = SpawnFromToken(hToken,strPipe,bSandbox);
	if (dwRes != ERROR_SUCCESS)
	{
		if (dwRes == ERROR_PRIVILEGE_NOT_HELD && unsafe_sandbox())
		{
			HANDLE hToken2;
			if (OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,&hToken2))
			{
				// Get the names associated with the user SID
				ACE_WString strUserName;
				ACE_WString strDomainName;

				if (GetNameFromToken(hToken2,strUserName,strDomainName) == ERROR_SUCCESS)
				{
					const wchar_t msg[] =
						L"OOServer is running under a user account that does not have the priviledges required to spawn processes as a different user.\n\n"
						L"Because the 'Unsafe' value is set in the registry, or a debugger is attached to OOServer, the new user process will be started under the user account '%s\\%s'\n\n"
						L"This is a security risk, and should only be allowed for debugging purposes, and only then if you really know what you are doing.";

					wchar_t szBuf[1024];
					ACE_OS::snprintf(szBuf,1023,msg,strDomainName.c_str(),strUserName.c_str());
					wchar_t szBuf2[1024];
					ACE_OS::snprintf(szBuf2,1023,L"%s\n\nDo you want to allow this?",szBuf);

					if (MessageBoxW(NULL,szBuf2,L"OOServer - Important security warning",MB_ICONEXCLAMATION | MB_YESNO | MB_SERVICE_NOTIFICATION | MB_DEFAULT_DESKTOP_ONLY | MB_DEFBUTTON2) == IDYES)
					{
						// Restrict the Token
						if (!RestrictToken(hToken2))
							dwRes = GetLastError();
						else
						{
							dwRes = SpawnFromToken(hToken2,strPipe,bSandbox);
							if (dwRes == ERROR_SUCCESS)
							{
								ACE_ERROR((LM_WARNING,L"%W",szBuf));
								ACE_OS::printf("\n\nYou chose to continue... on your head be it!\n\n");

								CloseHandle(m_hToken);
								DuplicateToken(hToken2,SecurityImpersonation,&m_hToken);
							}
						}
					}
				}

				CloseHandle(hToken2);
			}
		}
	}

	return (dwRes == ERROR_SUCCESS);
}

bool Root::SpawnedProcess::LogonSandboxUser(user_id_type& hToken)
{
	// Get the local machine registry
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Thread_Mutex> reg_root = Manager::get_registry();

	// Get the user name and pwd...
	ACE_INT64 key = 0;
	if (reg_root->open_key(key,"Server\\Sandbox",0) != 0)
		return false;

	ACE_WString strUName;
	ACE_WString strPwd;
	int err = reg_root->get_string_value(key,L"UserName",strUName);
	if (err != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: Failed to read sandbox username from registry: %C"),ACE_OS::strerror(err)),false);

	reg_root->get_string_value(key,L"Password",strPwd);

	if (!LogonUserW((LPWSTR)strUName.c_str(),NULL,(LPWSTR)strPwd.c_str(),LOGON32_LOGON_BATCH,LOGON32_PROVIDER_DEFAULT,&hToken))
	{
		LOG_FAILURE(GetLastError());
		return false;
	}

	// Restrict the Token
	if (!RestrictToken(hToken))
	{
		CloseHandle(hToken);
		LOG_FAILURE(GetLastError());
		return false;
	}

	// This might be needed for retricted tokens...
	DWORD dwRes = SetTokenDefaultDACL(hToken);
	if (dwRes != ERROR_SUCCESS)
	{
		CloseHandle(hToken);
		return false;
	}

	return true;
}

void Root::SpawnedProcess::CloseSandboxLogon(user_id_type hToken)
{
	CloseHandle(hToken);
}

bool Root::SpawnedProcess::CheckAccess(const char* pszFName, ACE_UINT32 mode, bool& bAllowed)
{
	PSECURITY_DESCRIPTOR pSD = NULL;
	DWORD cbNeeded = 0;
	if (!GetFileSecurityA(pszFName,DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,pSD,0,&cbNeeded) && GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
	{
		DWORD err = GetLastError();
		ACE_OS::last_error(err);
		ACE_OS::set_errno_to_last_error();
		return LOG_FAILURE(err);
	}

	pSD = static_cast<PSECURITY_DESCRIPTOR>(ACE_OS::malloc(cbNeeded));
	if (!pSD)
	{
		DWORD err = ERROR_OUTOFMEMORY;
		ACE_OS::last_error(err);
		ACE_OS::set_errno_to_last_error();
		return LOG_FAILURE(err);
	}

	if (!GetFileSecurityA(pszFName,DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,pSD,cbNeeded,&cbNeeded))
	{
		DWORD err = GetLastError();
		ACE_OS::last_error(err);
		ACE_OS::set_errno_to_last_error();
		return LOG_FAILURE(err);
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

	if (!bRes && err != ERROR_SUCCESS)
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
		LPVOID lpMsgBuf = 0;
		if (FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			err,
			0,
			(LPWSTR)&lpMsgBuf,
			0,	NULL))
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%W:%u: %W\n"),pszFile,nLine,(LPWSTR)lpMsgBuf));

			// Free the buffer.
			LocalFree(lpMsgBuf);
		}
		else
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%W:%u: Unknown system err %#x\n"),pszFile,nLine,err));
		}
	}

	return false;
}

bool Root::SpawnedProcess::InstallSandbox(int argc, ACE_TCHAR* argv[])
{
	ACE_WString strUName = L"_OMEGA_SANDBOX_USER_";
	ACE_WString strPwd = L"4th_(*%LGe895y^$N|2";

	if (argc>=1)
		strUName = ACE_TEXT_ALWAYS_WCHAR(argv[0]);
	if (argc>=2)
		strPwd = ACE_TEXT_ALWAYS_WCHAR(argv[1]);

	USER_INFO_2	info =
	{
		(LPWSTR)strUName.c_str(),  // usri2_name;
		(LPWSTR)strPwd.c_str(),    // usri2_password;
		0,                         // usri2_password_age;
		USER_PRIV_USER,            // usri2_priv;
		NULL,                      // usri2_home_dir;
		L"This account is used by the Omega Online sandbox to control access to system resources", // usri2_comment;
		UF_SCRIPT | UF_PASSWD_CANT_CHANGE | UF_DONT_EXPIRE_PASSWD | UF_NORMAL_ACCOUNT, // usri2_flags;
		NULL,                      // usri2_script_path;
		0,                         // usri2_auth_flags;
		L"Omega Online sandbox user account",   // usri2_full_name;
		L"This account is used by the Omega Online sandbox to control access to system resources", // usri2_usr_comment;
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
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Failed to add sandbox user: %#x\n"),err),false);

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
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Failed to lookup account SID\n")),false);
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
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Failed to lookup account SID\n")),false);
	}
	delete [] pszDName;

	if (use != SidTypeUser)
	{
		ACE_OS::free(pSid);
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Bad username\n")),false);
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
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Failed to access policy elements\n")),false);
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
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Failed to modify account priviledges\n")),false);
	}

	// Set the user name and pwd...
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Thread_Mutex> reg_root = Manager::get_registry();

	ACE_INT64 key = 0;
	if (reg_root->create_key(key,"Server\\Sandbox",false,0,0) != 0)
		return false;

	err = reg_root->set_string_value(key,L"UserName",info.usri2_name);
	if (err != 0)
		return false;

	err = reg_root->set_string_value(key,L"Password",info.usri2_password);
	if (err != 0)
		return false;

	if (bAddedUser)
		reg_root->set_integer_value(key,"AutoAdded",0,1);

	return true;
}

bool Root::SpawnedProcess::UninstallSandbox()
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Thread_Mutex> reg_root = Manager::get_registry();

	// Get the user name and pwd...
	ACE_INT64 key = 0;
	if (reg_root->open_key(key,"Server\\Sandbox",0) != 0)
		return true;

	ACE_WString strUName;
	if (reg_root->get_string_value(key,L"UserName",strUName) == 0)
	{
		ACE_CDR::LongLong bUserAdded = 0;
		reg_root->get_integer_value(key,"AutoAdded",0,bUserAdded);
		if (bUserAdded == 1)
			NetUserDel(NULL,strUName.c_str());
	}

	return true;
}

bool Root::SpawnedProcess::SecureFile(const ACE_CString& strFilename)
{
	// Specify the DACL to use.

	// Create a SID for the BUILTIN\Users group.
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
	DWORD dwRes = SetNamedSecurityInfoA(
		const_cast<LPSTR>(strFilename.c_str()), // name of the object
		SE_FILE_OBJECT,                          // type of object
		DACL_SECURITY_INFORMATION |              // change only the object's DACL
		PROTECTED_DACL_SECURITY_INFORMATION,     // And don't inherit!
		NULL, NULL,                              // don't change owner or group
		pACL,                                    // DACL specified
		NULL);                                   // don't change SACL

	LocalFree(pACL);

	return (ERROR_SUCCESS == dwRes);
}

void* Root::SpawnedProcess::GetTokenInfo(HANDLE hToken, TOKEN_INFORMATION_CLASS cls)
{
	DWORD dwLen = 0;
	if (!GetTokenInformation(hToken,cls,NULL,0,&dwLen) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return 0;

	void* pBuffer = ACE_OS::malloc(dwLen);
	if (!pBuffer)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return 0;
	}

	if (!GetTokenInformation(hToken,cls,pBuffer,dwLen,&dwLen))
	{
		ACE_OS::free(pBuffer);
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

bool Root::SpawnedProcess::Compare(user_id_type hToken)
{
	// Check the SIDs and priviledges are the same...
	TOKEN_GROUPS_AND_PRIVILEGES* pStats1 = static_cast<TOKEN_GROUPS_AND_PRIVILEGES*>(GetTokenInfo(hToken,TokenGroupsAndPrivileges));
	if (!pStats1)
		return false;

	TOKEN_GROUPS_AND_PRIVILEGES* pStats2 = static_cast<TOKEN_GROUPS_AND_PRIVILEGES*>(GetTokenInfo(m_hToken,TokenGroupsAndPrivileges));
	if (!pStats2)
	{
		ACE_OS::free(pStats1);
		return false;
	}

	bool bSame = (pStats1->SidCount==pStats2->SidCount &&
		pStats1->RestrictedSidCount==pStats2->RestrictedSidCount &&
		pStats1->PrivilegeCount==pStats2->PrivilegeCount &&
		MatchSids(pStats1->SidCount,pStats1->Sids,pStats2->Sids) &&
		MatchSids(pStats1->RestrictedSidCount,pStats1->RestrictedSids,pStats2->RestrictedSids) &&
		MatchPrivileges(pStats1->PrivilegeCount,pStats1->Privileges,pStats2->Privileges));

	ACE_OS::free(pStats1);
	ACE_OS::free(pStats2);

	return bSame;
}

bool Root::SpawnedProcess::IsSameUser(user_id_type hToken)
{
	TOKEN_USER* pUserInfo1 = static_cast<TOKEN_USER*>(GetTokenInfo(hToken,TokenUser));
	if (!pUserInfo1)
		return false;

	TOKEN_USER* pUserInfo2 = static_cast<TOKEN_USER*>(GetTokenInfo(m_hToken,TokenUser));
	if (!pUserInfo2)
	{
		ACE_OS::free(pUserInfo1);
		return false;
	}

	bool bSame = (EqualSid(pUserInfo1->User.Sid,pUserInfo2->User.Sid) == TRUE);

	ACE_OS::free(pUserInfo1);
	ACE_OS::free(pUserInfo2);

	return bSame;
}

ACE_CString Root::SpawnedProcess::GetRegistryHive()
{
	ACE_CString strRegistry;

	char szBuf[MAX_PATH] = {0};
	HRESULT hr;
	if (m_bSandbox)
		hr = SHGetFolderPathA(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
	else
		hr = SHGetFolderPathA(0,CSIDL_LOCAL_APPDATA,m_hToken,SHGFP_TYPE_DEFAULT,szBuf);

	if SUCCEEDED(hr)
	{
		char szBuf2[MAX_PATH] = {0};
		if (PathCombineA(szBuf2,szBuf,"Omega Online"))
		{
			if (PathFileExistsA(szBuf2) || ACE_OS::mkdir(szBuf2) == 0)
			{
				if (PathCombineA(szBuf,szBuf2,"user.regdb"))
					strRegistry = szBuf;
			}
		}
	}

	return strRegistry;
}

bool Root::SpawnedProcess::unsafe_sandbox()
{
	ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Thread_Mutex> reg_root = Manager::get_registry();

	// Get the user name and pwd...
	ACE_INT64 key = 0;
	if (reg_root->open_key(key,"Server\\Sandbox",0) != 0)
		return false;

	ACE_CDR::LongLong v = 0;
	if (reg_root->get_integer_value(key,"Unsafe",0,v) != 0)
	{
#if defined(OMEGA_DEBUG)
		return IsDebuggerPresent() ? true : false;
#else
		return false;
#endif
	}

	return (v == 1);
}

#endif // OMEGA_WIN32
