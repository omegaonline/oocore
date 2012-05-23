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
//  ***** THIS IS A SECURE MODULE *****
//
//  It will be run as Administrator/setuid root
//
//  Therefore it needs to be SAFE AS HOUSES!
//
//  Do not include anything unnecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"

#if defined(_WIN32)

#include "RootManager.h"

#include <ntsecapi.h>
#include <shlwapi.h>
#include <shlobj.h>

#ifndef PROTECTED_DACL_SECURITY_INFORMATION
#define PROTECTED_DACL_SECURITY_INFORMATION  (0x80000000L)
#endif

bool Root::platform_init()
{
	if (!is_debug())
	{
		// Change working directory to the location of the executable (we know it's valid!)
		wchar_t szPath[MAX_PATH];
		if (!GetModuleFileNameW(NULL,szPath,MAX_PATH))
			LOG_ERROR_RETURN(("GetModuleFileName failed: %s",OOBase::system_error_text()),false);

		// Strip off our name
		PathUnquoteSpacesW(szPath);
		PathRemoveFileSpecW(szPath);

		if (!SetCurrentDirectoryW(szPath))
			LOG_ERROR_RETURN(("SetCurrentDirectory(%s) failed: %s",szPath,OOBase::system_error_text()),false);
	}
	return true;
}

/*bool Root::Manager::secure_file(const char* pszFile, bool bPublicRead)
{
    // Create a SID for the BUILTIN\Users group.
    PSID pSid;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = {SECURITY_NT_AUTHORITY};
    if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_USERS,
        0, 0, 0, 0, 0, 0,
        &pSid))
    {
        LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::system_error_text().c_str()),false);
    }
    OOBase::SmartPtr<void,OOBase::Win32::SIDDestructor<void> > pSIDUsers(pSid);

    // Create a SID for the BUILTIN\Administrators group.
    if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pSid))
    {
        LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::system_error_text().c_str()),false);
    }
    OOBase::SmartPtr<void,OOBase::Win32::SIDDestructor<void> > pSIDAdmin(pSid);

    const int NUM_ACES  = 2;
    EXPLICIT_ACCESSW ea[NUM_ACES] = {0};

    if (bPublicRead)
    {
        // Set read access for Users.
        ea[0].grfAccessPermissions = FILE_GENERIC_READ;
        ea[0].grfAccessMode = SET_ACCESS;
        ea[0].grfInheritance = NO_INHERITANCE;
        ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea[0].Trustee.ptstrName = (LPWSTR)pSIDUsers;
    }
    else
    {
        ea[0].grfAccessPermissions = 0;//FILE_GENERIC_READ;
        ea[0].grfAccessMode = REVOKE_ACCESS;
        ea[0].grfInheritance = NO_INHERITANCE;
        ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea[0].Trustee.ptstrName = (LPWSTR)pSIDUsers;
    }

    // Set full control for Administrators.
    ea[1].grfAccessPermissions = GENERIC_ALL;
    ea[1].grfAccessMode = GRANT_ACCESS;
    ea[1].grfInheritance = NO_INHERITANCE;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[1].Trustee.ptstrName = (LPWSTR)pSIDAdmin;

    PACL pACL = 0;
    DWORD dwErr = SetEntriesInAclW(NUM_ACES,ea,NULL,&pACL);
    if (dwErr != 0)
        LOG_ERROR_RETURN(("SetEntriesInAclW failed: %s",OOBase::system_error_text(dwErr).c_str()),false);

    OOBase::SmartPtr<ACL,OOBase::Win32::LocalAllocDestructor<ACL> > ptrACL = pACL;

    // Try to modify the object's DACL.
    dwErr = SetNamedSecurityInfoA(
        pszFile,                                 // name of the object
        SE_FILE_OBJECT,                          // type of object
        DACL_SECURITY_INFORMATION |              // change only the object's DACL
        PROTECTED_DACL_SECURITY_INFORMATION,     // And don't inherit!
        NULL, NULL,                              // don't change owner or group
        pACL,                                    // DACL specified
        NULL);                                   // don't change SACL

    if (dwErr != 0)
        LOG_ERROR_RETURN(("SetNamedSecurityInfoA failed: %s",OOBase::system_error_text(dwErr).c_str()),false);

    return true;
}*/

bool Root::Manager::load_config_i(const OOBase::CmdArgs::results_t& cmd_args)
{
	// Read from registry
	HKEY hKey = 0;
	LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE,"Software\\Omega Online\\OOServer",0,KEY_READ,&hKey);
	if (lRes != ERROR_SUCCESS)
	{
		if (lRes == ERROR_FILE_NOT_FOUND)
			OOBase::Logger::log(OOBase::Logger::Warning,"Missing registry key: HKEY_LOCAL_MACHINE\\Software\\Omega Online\\OOServer");
		else
			LOG_ERROR(("Failed to open config registry key: %s",OOBase::system_error_text(lRes)));
	}
	else
	{
		// Loop pulling out registry values
		for (DWORD dwIndex=0;; ++dwIndex)
		{
			char szBuf[1024];
			DWORD dwNameLen = sizeof(szBuf);
			DWORD dwType = 0;
			DWORD dwValLen = 0;
			lRes = RegEnumValueA(hKey,dwIndex,szBuf,&dwNameLen,NULL,&dwType,NULL,&dwValLen);
			if (lRes == ERROR_NO_MORE_ITEMS)
				break;
			else if (lRes != ERROR_SUCCESS)
			{
				RegCloseKey(hKey);
				LOG_ERROR_RETURN(("RegEnumValueA failed: %s",OOBase::system_error_text(lRes)),false);
			}

			// Skip anything starting with #
			if (dwValLen>=1 && szBuf[0]=='#')
				continue;

			OOBase::String value,key;
			lRes = key.assign(szBuf,dwNameLen);
			if (lRes != 0)
				LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(lRes)),false);

			++dwNameLen;

			if (dwType == REG_DWORD)
			{
				DWORD dwVal = 0;
				DWORD dwLen = sizeof(dwVal);
				lRes = RegEnumValueA(hKey,dwIndex,szBuf,&dwNameLen,NULL,NULL,(LPBYTE)&dwVal,&dwLen);
				if (lRes != ERROR_SUCCESS)
					LOG_ERROR_RETURN(("RegQueryValueA failed: %s",OOBase::system_error_text(lRes)),false);

				lRes = value.printf("%d",static_cast<int>(dwVal));
				if (lRes != 0)
					LOG_ERROR_RETURN(("Failed to format string: %s",OOBase::system_error_text(lRes)),false);
			}
			else if (dwType == REG_SZ || dwType == REG_EXPAND_SZ)
			{
				++dwValLen;
				OOBase::SmartPtr<char,OOBase::LocalAllocator> buf;
				if (!buf.allocate(dwValLen+1))
					LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text()),false);

				lRes = RegEnumValueA(hKey,dwIndex,szBuf,&dwNameLen,NULL,NULL,(LPBYTE)(char*)buf,&dwValLen);
				if (lRes != ERROR_SUCCESS)
					LOG_ERROR_RETURN(("RegQueryValueA failed: %s",OOBase::system_error_text(lRes)),false);

				if (dwType == REG_EXPAND_SZ)
				{
					DWORD dwExpLen = ExpandEnvironmentStringsA(buf,szBuf,sizeof(szBuf));
					if (dwExpLen == 0)
						LOG_ERROR_RETURN(("ExpandEnvironmentStringsA failed: %s",OOBase::system_error_text()),false);
					else if (dwExpLen <= sizeof(szBuf))
						lRes = value.assign(szBuf,dwExpLen-1);
					else
					{
						OOBase::SmartPtr<char,OOBase::LocalAllocator> buf3;
						if (!buf3.allocate(dwExpLen+1))
							LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text()),false);

						if (!ExpandEnvironmentStringsA(buf,buf3,dwExpLen))
							LOG_ERROR_RETURN(("ExpandEnvironmentStringsA failed: %s",OOBase::system_error_text()),false);

						lRes = value.assign(buf3,dwExpLen-1);
					}

					if (lRes != 0)
						LOG_ERROR_RETURN(("Failed to format string: %s",OOBase::system_error_text(lRes)),false);
				}
				else
					value.assign(buf,dwValLen-1);
			}
			else
			{
				LOG_ERROR(("Registry value %s is of invalid type",key.c_str()));
				continue;
			}

			if (!key.empty())
			{
				OOBase::String* v = m_config_args.find(key);
				if (!v)
				{
					lRes = m_config_args.insert(key,value);
					if (lRes != 0)
						LOG_ERROR_RETURN(("Failed to insert config string: %s",OOBase::system_error_text(lRes)),false);
				}
			}
		}

		RegCloseKey(hKey);
	}

	// Load any config file now...
	size_t f = m_config_args.find_first("conf-file");
	if (f != m_config_args.npos)
	{
		OOBase::Logger::log(OOBase::Logger::Information,"Using config file: %s",cmd_args.at(f)->c_str());

		if (!load_config_file(cmd_args.at(f)->c_str()))
			return false;
	}

	// Now set some defaults...
	if (!m_config_args.exists("regdb_path"))
	{
		// This does need to be ASCII
		char szBuf[MAX_PATH] = {0};
		HRESULT hr = SHGetFolderPathA(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
		if FAILED(hr)
			LOG_ERROR_RETURN(("SHGetFolderPathA failed: %s",OOBase::system_error_text()),false);

		if (!PathAppendA(szBuf,"Omega Online"))
			LOG_ERROR_RETURN(("PathAppendA failed: %s",OOBase::system_error_text()),false);

		if (!PathFileExistsA(szBuf))
			LOG_ERROR_RETURN(("%s does not exist.",szBuf),false);

		OOBase::String v,k;
		int err = k.assign("regdb_path");
		if (err == 0)
			err = v.assign(szBuf);
		if (err == 0)
			err = m_config_args.insert(k,v);
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to insert string: %s",OOBase::system_error_text()),false);
	}

	if (!m_config_args.exists("binary_path"))
	{
		// Get our module name
		wchar_t wszPath[MAX_PATH] = {0};
		if (!GetModuleFileNameW(NULL,wszPath,MAX_PATH))
			LOG_ERROR_RETURN(("GetModuleFileName failed: %s",OOBase::system_error_text()),false);

		// Strip off our name
		PathUnquoteSpacesW(wszPath);
		PathRemoveFileSpecW(wszPath);
		if (!PathAddBackslashW(wszPath))
			LOG_ERROR_RETURN(("PathAddBackslash failed: %s",OOBase::system_error_text()),false);

		char szPath[MAX_PATH * 2] = {0};
		if (!WideCharToMultiByte(CP_UTF8,0,wszPath,-1,szPath,sizeof(szPath),NULL,NULL))
			LOG_ERROR_RETURN(("WideCharToMultiByte failed: %s",OOBase::system_error_text()),false);

		OOBase::String v,k;
		int err = k.assign("binary_path");
		if (err == 0)
			err = v.assign(szPath);
		if (err == 0)
			err = m_config_args.insert(k,v);
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to insert string: %s",OOBase::system_error_text()),false);
	}

	return true;
}

bool Root::Manager::start_client_acceptor()
{
	const int NUM_ACES = 3;
	EXPLICIT_ACCESSW ea[NUM_ACES] = { {0}, {0}, {0} };

	PSID pSID;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = {SECURITY_NT_AUTHORITY};
	if (!AllocateAndInitializeSid(&SIDAuthNT, 1,
								  SECURITY_LOCAL_SYSTEM_RID,
								  0, 0, 0, 0, 0, 0, 0,
								  &pSID))
	{
		LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::system_error_text()),false);
	}

	OOBase::SmartPtr<void,OOBase::Win32::SIDDestructor> pSIDSystem(pSID);

	// Set full control for the creating process SID
	ea[0].grfAccessPermissions = FILE_ALL_ACCESS;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[0].Trustee.ptstrName = (LPWSTR)pSIDSystem;  // Don't use CREATOR/OWNER, it doesn't work with multiple pipe instances...
		
	// Get the current user's Logon SID
	OOBase::Win32::SmartHandle hProcessToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
		LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::system_error_text()),false);

	// Get the logon SID of the Token
	OOBase::SmartPtr<void,OOBase::LocalAllocator> ptrSIDLogon;
	if (OOBase::Win32::GetLogonSID(hProcessToken,ptrSIDLogon) == ERROR_SUCCESS)
	{
		// Use logon sid instead...
		ea[0].Trustee.ptstrName = (LPWSTR)ptrSIDLogon;  // Don't use CREATOR/OWNER, it doesn't work with multiple pipe instances...
	}

	// Create a SID for the EVERYONE group.
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = {SECURITY_WORLD_SID_AUTHORITY};
	if (!AllocateAndInitializeSid(&SIDAuthWorld, 1,
								  SECURITY_WORLD_RID,
								  0, 0, 0, 0, 0, 0, 0,
								  &pSID))
	{
		LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::system_error_text()),false);
	}
	OOBase::SmartPtr<void,OOBase::Win32::SIDDestructor> pSIDEveryone(pSID);

	// Set read/write access for EVERYONE
	ea[1].grfAccessPermissions = FILE_GENERIC_READ | FILE_WRITE_DATA;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[1].Trustee.ptstrName = (LPWSTR)pSIDEveryone;

	// Create a SID for the Network group.
	if (!AllocateAndInitializeSid(&SIDAuthNT, 1,
								  SECURITY_NETWORK_RID,
								  0, 0, 0, 0, 0, 0, 0,
								  &pSID))
	{
		LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::system_error_text()),false);
	}
	OOBase::SmartPtr<void,OOBase::Win32::SIDDestructor> pSIDNetwork(pSID);

	// Deny all to NETWORK
	ea[2].grfAccessPermissions = FILE_ALL_ACCESS;
	ea[2].grfAccessMode = DENY_ACCESS;
	ea[2].grfInheritance = NO_INHERITANCE;
	ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[2].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[2].Trustee.ptstrName = (LPWSTR)pSIDNetwork;

	// Create a new ACL
	DWORD dwErr = m_sd.SetEntriesInAcl(NUM_ACES,ea,NULL);
	if (dwErr != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("SetEntriesInAcl failed: %s",OOBase::system_error_text(dwErr)),false);

	// Create a new security descriptor
	m_sa.nLength = sizeof(m_sa);
	m_sa.bInheritHandle = FALSE;
	m_sa.lpSecurityDescriptor = m_sd.descriptor();

	const char* pipe_name = "OmegaOnline";
	int err = 0;
	m_client_acceptor = m_proactor->accept_local(this,&accept_client,pipe_name,err,&m_sa);
	if (err != 0)
		LOG_ERROR_RETURN(("Proactor::accept_local failed: '%s' %s",pipe_name,OOBase::system_error_text(err)),false);

	return true;
}

#endif // _WIN32
