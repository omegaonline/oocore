///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOSvrBase, the Omega Online Base library.
//
// OOSvrBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOSvrBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "SecurityWin32.h"

#if defined(_WIN32)

#include <shlwapi.h>

OOSvrBase::Win32::sec_descript_t::sec_descript_t() :
	m_pACL(NULL), m_psd(NULL)
{
	// Create a new security descriptor
	m_psd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (m_psd == NULL)
		OOBase_CallCriticalFailure(GetLastError());

	// Initialize a security descriptor.
	if (!InitializeSecurityDescriptor((PSECURITY_DESCRIPTOR)m_psd.value(),SECURITY_DESCRIPTOR_REVISION))
		OOBase_CallCriticalFailure(GetLastError());
}

OOSvrBase::Win32::sec_descript_t::~sec_descript_t()
{
}

DWORD OOSvrBase::Win32::sec_descript_t::SetEntriesInAcl(ULONG cCountOfExplicitEntries, PEXPLICIT_ACCESSW pListOfExplicitEntries, PACL OldAcl)
{
	if (m_pACL)
		m_pACL = 0;

	PACL pACL;
	DWORD dwErr = ::SetEntriesInAclW(cCountOfExplicitEntries,pListOfExplicitEntries,OldAcl,&pACL);
	if (ERROR_SUCCESS != dwErr)
		return dwErr;

	m_pACL = pACL;

	// Add the ACL to the SD
	if (!SetSecurityDescriptorDacl((PSECURITY_DESCRIPTOR)m_psd.value(),TRUE,m_pACL.value(),FALSE))
		return GetLastError();

	return ERROR_SUCCESS;
}

DWORD OOSvrBase::Win32::GetNameFromToken(HANDLE hToken, std::wstring& strUserName, std::wstring& strDomainName)
{
	// Find out all about the user associated with hToken
	OOBase::SmartPtr<TOKEN_USER,OOBase::FreeDestructor<TOKEN_USER> > ptrUserInfo = static_cast<TOKEN_USER*>(GetTokenInfo(hToken,TokenUser));
	if (!ptrUserInfo)
		return GetLastError();
	
	SID_NAME_USE name_use;
	DWORD dwUNameSize = 0;
	DWORD dwDNameSize = 0;
	LookupAccountSidW(NULL,ptrUserInfo->User.Sid,NULL,&dwUNameSize,NULL,&dwDNameSize,&name_use);
	if (dwUNameSize == 0)
		return GetLastError();
		
	OOBase::SmartPtr<wchar_t,OOBase::ArrayDestructor<wchar_t> > ptrUserName = 0;
	OOBASE_NEW(ptrUserName,wchar_t[dwUNameSize]);
	if (!ptrUserName)
		return ERROR_OUTOFMEMORY;
		
	OOBase::SmartPtr<wchar_t,OOBase::ArrayDestructor<wchar_t> > ptrDomainName = 0;
	if (dwDNameSize)
	{
		OOBASE_NEW(ptrDomainName,wchar_t[dwDNameSize]);
		if (!ptrDomainName)
			return ERROR_OUTOFMEMORY;
	}
				
	if (!LookupAccountSidW(NULL,ptrUserInfo->User.Sid,ptrUserName.value(),&dwUNameSize,ptrDomainName.value(),&dwDNameSize,&name_use))
		return GetLastError();
				
	strUserName = ptrUserName.value();
	strDomainName = ptrDomainName.value();
		
	return ERROR_SUCCESS;
}

DWORD OOSvrBase::Win32::LoadUserProfileFromToken(HANDLE hToken, HANDLE& hProfile)
{
	// Get the names associated with the user SID
	std::wstring strUserName;
	std::wstring strDomainName;

	DWORD err = GetNameFromToken(hToken,strUserName,strDomainName);
	if (err != ERROR_SUCCESS)
		return err;

	// Lookup a DC for pszDomain
	std::wstring strDCName;
	LPWSTR pszDCName = NULL;
	if (NetGetAnyDCName(NULL,strDomainName.empty() ? NULL : strDomainName.c_str(),(LPBYTE*)&pszDCName) == NERR_Success)
	{
		strDCName = pszDCName;
		NetApiBufferFree(pszDCName);
	}

	// Try to find the user's profile path...
	std::wstring strProfilePath;
	USER_INFO_3* pInfo = NULL;
	if (NetUserGetInfo(strDCName.empty() ? NULL : strDCName.c_str(),strUserName.c_str(),3,(LPBYTE*)&pInfo) == NERR_Success)
	{
		if (pInfo->usri3_profile)
			strProfilePath = pInfo->usri3_profile;

		NetApiBufferFree(pInfo);
	}

	// Load the Users Profile
	PROFILEINFOW profile_info = {0};
	profile_info.dwSize = sizeof(PROFILEINFOW);
	profile_info.dwFlags = PI_NOUI;
	profile_info.lpUserName = (WCHAR*)strUserName.c_str();

	if (!strProfilePath.empty())
		profile_info.lpProfilePath = (WCHAR*)strProfilePath.c_str();

	if (!strDCName.empty())
		profile_info.lpServerName = (WCHAR*)strDCName.c_str();

	if (!LoadUserProfileW(hToken,&profile_info))
		return GetLastError();
	
	hProfile = profile_info.hProfile;
	return ERROR_SUCCESS;
}

DWORD OOSvrBase::Win32::GetLogonSID(HANDLE hToken, OOBase::SmartPtr<void,OOBase::FreeDestructor<void> >& pSIDLogon)
{
	// Get the logon SID of the Token
	OOBase::SmartPtr<TOKEN_GROUPS,OOBase::FreeDestructor<TOKEN_GROUPS> > ptrGroups = static_cast<TOKEN_GROUPS*>(GetTokenInfo(hToken,TokenGroups));
	if (!ptrGroups)
		return GetLastError();

	// Loop through the groups to find the logon SID
	for (DWORD dwIndex = 0; dwIndex < ptrGroups->GroupCount; ++dwIndex)
	{
		if ((ptrGroups->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID)
		{
			// Found the logon SID...
			if (IsValidSid(ptrGroups->Groups[dwIndex].Sid))
			{
				DWORD dwLen = GetLengthSid(ptrGroups->Groups[dwIndex].Sid);
				pSIDLogon = static_cast<PSID>(malloc(dwLen));
				if (!pSIDLogon)
					return ERROR_OUTOFMEMORY;
				
				if (!CopySid(dwLen,pSIDLogon.value(),ptrGroups->Groups[dwIndex].Sid))
					return GetLastError();
					
				return ERROR_SUCCESS;
			}
		}
	}

	return ERROR_INVALID_SID;
}

DWORD OOSvrBase::Win32::SetTokenDefaultDACL(HANDLE hToken)
{
	// Get the current Default DACL
	OOBase::SmartPtr<TOKEN_DEFAULT_DACL,OOBase::FreeDestructor<TOKEN_DEFAULT_DACL> > ptrDef_dacl = static_cast<TOKEN_DEFAULT_DACL*>(GetTokenInfo(hToken,TokenDefaultDacl));
	if (!ptrDef_dacl)
		return ERROR_OUTOFMEMORY;

	// Get the logon SID of the Token
	OOBase::SmartPtr<void,OOBase::FreeDestructor<void> > ptrSIDLogon = 0;
	DWORD dwRes = GetLogonSID(hToken,ptrSIDLogon);
	if (dwRes != ERROR_SUCCESS)
		return dwRes;
	
	const int NUM_ACES = 1;
	EXPLICIT_ACCESSW ea[NUM_ACES] = {0};
	
	// Set maximum access for the logon SID
	ea[0].grfAccessPermissions = GENERIC_ALL;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[0].Trustee.ptstrName = (LPWSTR)ptrSIDLogon.value();

	TOKEN_DEFAULT_DACL def_dacl = {0};
	dwRes = SetEntriesInAclW(NUM_ACES,ea,ptrDef_dacl->DefaultDacl,&def_dacl.DefaultDacl);
	if (dwRes != ERROR_SUCCESS)
		return dwRes;

	// Now set the token default DACL
	if (!SetTokenInformation(hToken,TokenDefaultDacl,&def_dacl,sizeof(def_dacl)))
		dwRes = GetLastError();

	LocalFree(def_dacl.DefaultDacl);

	return dwRes;
}

DWORD OOSvrBase::Win32::EnableUserAccessToDir(const wchar_t* pszPath, const TOKEN_USER* pUser)
{
	wchar_t szPath[MAX_PATH] = {0};
	PathCanonicalizeW(szPath,pszPath);
    PathRemoveFileSpecW(szPath);

	PACL pACL = 0;
	PSECURITY_DESCRIPTOR pSD = 0;
	DWORD dwRes = GetNamedSecurityInfoW(szPath,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,&pACL,NULL,&pSD);
	if (dwRes != ERROR_SUCCESS)
		return dwRes;

	OOBase::SmartPtr<void,OOBase::Win32::LocalAllocDestructor<void> > ptrSD = pSD;

	static const int NUM_ACES = 1;
	EXPLICIT_ACCESSW ea[NUM_ACES] = {0};
	
	// Set maximum access for the logon SID
	ea[0].grfAccessPermissions = GENERIC_ALL;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = OBJECT_INHERIT_ACE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[0].Trustee.ptstrName = (LPWSTR)pUser->User.Sid;

	PACL pACLNew;
	dwRes = SetEntriesInAclW(NUM_ACES,ea,pACL,&pACLNew);
	if (dwRes != ERROR_SUCCESS)
		return dwRes;

	OOBase::SmartPtr<ACL,OOBase::Win32::LocalAllocDestructor<ACL> > ptrACLNew = pACLNew;

	return SetNamedSecurityInfoW(szPath,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,NULL,NULL,pACLNew,NULL);
}

DWORD OOSvrBase::Win32::RestrictToken(HANDLE& hToken)
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
			return GetLastError();

		CloseHandle(hToken);
		hToken = hNewToken;
	}

	if (os.dwMajorVersion > 5)
	{
		// Vista - use UAC as well...
	}

	return ERROR_SUCCESS;
}

void* OOSvrBase::Win32::GetTokenInfo(HANDLE hToken, TOKEN_INFORMATION_CLASS cls)
{
	DWORD dwLen = 0;
	if (!GetTokenInformation(hToken,cls,NULL,0,&dwLen) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
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

bool OOSvrBase::Win32::MatchSids(ULONG count, PSID_AND_ATTRIBUTES pSids1, PSID_AND_ATTRIBUTES pSids2)
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

bool OOSvrBase::Win32::MatchPrivileges(ULONG count, PLUID_AND_ATTRIBUTES Privs1, PLUID_AND_ATTRIBUTES Privs2)
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

#endif // _WIN32
