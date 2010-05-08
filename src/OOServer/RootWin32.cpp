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
//  Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "RootManager.h"

#if defined(_WIN32)

#include "../OOBase/SecurityWin32.h"

#include <ntsecapi.h>
#include <shlwapi.h>
#include <shlobj.h>

#ifndef PROTECTED_DACL_SECURITY_INFORMATION
#define PROTECTED_DACL_SECURITY_INFORMATION  (0x80000000L)
#endif

/*bool Root::Manager::secure_file(const std::string& strFile, bool bPublicRead)
{
    std::wstring strFilename = OOBase::from_utf8(strFile.c_str());

    // Create a SID for the BUILTIN\Users group.
    PSID pSid;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_USERS,
        0, 0, 0, 0, 0, 0,
        &pSid))
    {
        LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::Win32::FormatMessage().c_str()),false);
    }
    OOBase::SmartPtr<void,OOBase::Win32::SIDDestructor<void> > pSIDUsers(pSid);

    // Create a SID for the BUILTIN\Administrators group.
    if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pSid))
    {
        LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::Win32::FormatMessage().c_str()),false);
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
        LOG_ERROR_RETURN(("SetEntriesInAclW failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),false);

    OOBase::SmartPtr<ACL,OOBase::Win32::LocalAllocDestructor<ACL> > ptrACL = pACL;

    // Try to modify the object's DACL.
    dwErr = SetNamedSecurityInfoW(
        const_cast<wchar_t*>(strFilename.c_str()), // name of the object
        SE_FILE_OBJECT,                          // type of object
        DACL_SECURITY_INFORMATION |              // change only the object's DACL
        PROTECTED_DACL_SECURITY_INFORMATION,     // And don't inherit!
        NULL, NULL,                              // don't change owner or group
        pACL,                                    // DACL specified
        NULL);                                   // don't change SACL

    if (dwErr != 0)
        LOG_ERROR_RETURN(("SetNamedSecurityInfoW failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),false);

    return true;
}*/

bool Root::Manager::load_config()
{
	try
	{
		// Clear current entries
		m_config_args.clear();

		// Insert platform defaults
		wchar_t szBuf[MAX_PATH] = {0};
		HRESULT hr = SHGetFolderPathW(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
		if FAILED(hr)
			LOG_ERROR_RETURN(("SHGetFolderPathW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

		if (!PathAppendW(szBuf,L"Omega Online"))
			LOG_ERROR_RETURN(("PathAppendW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

		if (!PathFileExistsW(szBuf))
			LOG_ERROR_RETURN(("%s does not exist.",OOBase::to_utf8(szBuf).c_str()),false);

		std::string dir = OOBase::to_utf8(szBuf).c_str();
		if (*dir.rbegin() != '\\')
			dir += '\\';

		m_config_args["regdb_path"] = dir;

		std::map<std::string,std::string>::const_iterator f = m_cmd_args.find("conf-file");
		if (f != m_cmd_args.end())
			return load_config_file(f->second);

		// Read from registry
		HKEY hKey = 0;
		LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE,"Software\\Omega Online\\OOServer",0,KEY_READ,&hKey);
		if (lRes == ERROR_FILE_NOT_FOUND)
			return true;
		else if (lRes != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("RegOpenKeyExA failed: %s",OOBase::Win32::FormatMessage(lRes).c_str()),false);

		try
		{
			// Loop pulling out registry values
			for (DWORD dwIndex=0;; ++dwIndex)
			{
				char valName[16383 + 1];
				DWORD dwNameLen = 16383 + 1;
				DWORD dwType = 0;
				DWORD dwValLen = 0;
				lRes = RegEnumValueA(hKey,dwIndex,valName,&dwNameLen,NULL,&dwType,NULL,&dwValLen);
				if (lRes == ERROR_NO_MORE_ITEMS)
					break;
				else if (lRes != ERROR_SUCCESS)
				{
					RegCloseKey(hKey);
					LOG_ERROR_RETURN(("RegEnumValueA failed: %s",OOBase::Win32::FormatMessage(lRes).c_str()),false);
				}

				// Skip anything starting with #
				if (dwValLen>=1 && valName[0]=='#')
					continue;

				std::string value,key(valName,dwNameLen);
				++dwNameLen;

				if (dwType == REG_DWORD)
				{
					DWORD dwVal = 0;
					DWORD dwLen = sizeof(dwVal);
					lRes = RegEnumValueA(hKey,dwIndex,valName,&dwNameLen,NULL,NULL,(LPBYTE)&dwVal,&dwLen);
					if (lRes != ERROR_SUCCESS)
					{
						LOG_ERROR(("RegQueryValueA failed: %s",OOBase::Win32::FormatMessage(lRes).c_str()));
						continue;
					}

					std::ostringstream os;
					os.imbue(std::locale::classic());
					os << dwVal;
					value = os.str();
				}
				else if (dwType == REG_SZ || dwType == REG_EXPAND_SZ)
				{
					++dwValLen;
					OOBase::SmartPtr<char,OOBase::FreeDestructor<char> > buf(static_cast<char*>(malloc(dwValLen)));
					if (!buf)
					{
						LOG_ERROR(("Out of memory"));
						continue;
					}

					lRes = RegEnumValueA(hKey,dwIndex,valName,&dwNameLen,NULL,NULL,(LPBYTE)(char*)buf,&dwValLen);
					if (lRes != ERROR_SUCCESS)
					{
						LOG_ERROR(("RegQueryValueA failed: %s",OOBase::Win32::FormatMessage(lRes).c_str()));
						continue;
					}

					if (dwType == REG_EXPAND_SZ)
					{
						char buf2[1024] = {0};
						DWORD dwExpLen = ExpandEnvironmentStringsA(buf,buf2,1023);
						if (dwExpLen == 0)
						{
							DWORD dwErr = GetLastError();
							LOG_ERROR(("ExpandEnvironmentStringsA failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()));
							continue;
						}
						else if (dwExpLen <= 1023)
							value.assign(buf2,dwExpLen);
						else
						{
							OOBase::SmartPtr<char,OOBase::FreeDestructor<char> > buf3(static_cast<char*>(malloc(dwExpLen+1)));
							if (!buf)
							{
								LOG_ERROR(("Out of memory"));
								continue;
							}

							if (!ExpandEnvironmentStringsA(buf,buf3,dwExpLen+1))
							{
								DWORD dwErr = GetLastError();
								LOG_ERROR(("ExpandEnvironmentStringsA failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()));
								continue;
							}

							value.assign(buf3,dwExpLen);
						}
					}
					else
						value.assign(buf,dwValLen);
				}
				else
				{
					LOG_ERROR(("Registry value %s is of invalid type",key.c_str()));
					continue;
				}

				if (!key.empty())
					m_config_args[key] = value;
			}

			RegCloseKey(hKey);

			return true;
		}
		catch (...)
		{
			RegCloseKey(hKey);
			throw;
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_RETURN(("std::exception thrown %s",e.what()),false);
	}
}

namespace
{
	static HANDLE s_hEvent;
	static SERVICE_STATUS_HANDLE s_ssh;

	static BOOL WINAPI control_c(DWORD)
	{
		// Just stop!
		SetEvent(s_hEvent);
		return TRUE;
	}

	static void shell_wait()
	{
		// Create the wait event
		s_hEvent = CreateEventW(NULL,TRUE,FALSE,NULL);
		if (!s_hEvent)
		{
			LOG_ERROR(("CreateEventW failed: %s",OOBase::Win32::FormatMessage().c_str()));
			return;
		}

		if (!SetConsoleCtrlHandler(control_c,TRUE))
			LOG_ERROR(("SetConsoleCtrlHandler failed: %s",OOBase::Win32::FormatMessage().c_str()));
		else
		{
			// Wait for the event to be signalled
			DWORD dwWait = WaitForSingleObject(s_hEvent,INFINITE);
			if (dwWait != WAIT_OBJECT_0)
				LOG_ERROR(("WaitForSingleObject failed: %s",OOBase::Win32::FormatMessage().c_str()));
		}

		CloseHandle(s_hEvent);
	}

	static VOID WINAPI ServiceCtrl(DWORD dwControl)
	{
		SERVICE_STATUS ss = {0};
		ss.dwControlsAccepted = SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN;
		ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		ss.dwCurrentState = SERVICE_RUNNING;

		switch (dwControl)
		{
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			ss.dwCurrentState = SERVICE_STOP_PENDING;
			break;

		default:
			break;
		}

		// Set the status of the service
		SetServiceStatus(s_ssh,&ss);

		// Tell service_main thread to stop.
		if (dwControl == SERVICE_CONTROL_STOP || dwControl == SERVICE_CONTROL_SHUTDOWN)
		{
			if (!SetEvent(s_hEvent))
				OOBase_CallCriticalFailure(GetLastError());
		}
	}

	static VOID WINAPI ServiceMain(DWORD /*dwArgc*/, LPWSTR* lpszArgv)
	{
		// Create the wait event
		s_hEvent = CreateEventW(NULL,TRUE,FALSE,NULL);
		if (!s_hEvent)
		{
			LOG_ERROR(("CreateEventW failed: %s",OOBase::Win32::FormatMessage().c_str()));
			return;
		}

		SERVICE_STATUS ss = {0};
		ss.dwControlsAccepted = SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN;
		ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

		// Register the service ctrl handler.
		s_ssh = RegisterServiceCtrlHandlerW(lpszArgv[0],&ServiceCtrl);
		if (!s_ssh)
			LOG_ERROR(("RegisterServiceCtrlHandlerW failed: %s",OOBase::Win32::FormatMessage().c_str()));
		else
		{
			ss.dwCurrentState = SERVICE_RUNNING;
			SetServiceStatus(s_ssh,&ss);

			// Wait for the event to be signalled
			DWORD dwWait = WaitForSingleObject(s_hEvent,INFINITE);
			if (dwWait != WAIT_OBJECT_0)
				LOG_ERROR(("WaitForSingleObject failed: %s",OOBase::Win32::FormatMessage().c_str()));

			ss.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(s_ssh,&ss);
		}

		CloseHandle(s_hEvent);
	}
}

bool Root::Manager::wait_for_quit()
{
	SERVICE_TABLE_ENTRYW ste[] =
	{
		{L"", &ServiceMain },
		{NULL, NULL}
	};

	if (!StartServiceCtrlDispatcherW(ste))
	{
		DWORD err = GetLastError();
		if (err == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
		{
			// We are running from the shell...
			shell_wait();
		}
		else
			LOG_ERROR(("StartServiceCtrlDispatcherW failed: %s",OOBase::Win32::FormatMessage().c_str()));
	}

	// By the time we get here, it's all over
	return true;
}

namespace OOBase
{
	// This is the critical failure hook
	void CriticalFailure(const char* msg)
	{
		OOSvrBase::Logger::log(OOSvrBase::Logger::Error,msg);

		// Die horribly now!
		abort();
		TerminateProcess(GetCurrentProcess(),EXIT_FAILURE);
	}
}

#endif // _WIN32
