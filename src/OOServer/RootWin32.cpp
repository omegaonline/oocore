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

#include "OOServer_Root.h"
#include "RootManager.h"

#if defined(_WIN32)

#include "../OOBase/SecurityWin32.h"

#include <ntsecapi.h>
#include <shlwapi.h>
#include <shlobj.h>

#ifndef PROTECTED_DACL_SECURITY_INFORMATION
#define PROTECTED_DACL_SECURITY_INFORMATION	 (0x80000000L)
#endif

#define SERVICE_NAME L"OOServer"

bool Root::Manager::platform_install(int /*argc*/, char* /*argv*/[])
{
	wchar_t szPath[MAX_PATH]; 
	if (!GetModuleFileNameW(NULL,szPath,MAX_PATH))
		LOG_ERROR_RETURN(("GetModuleFileNameW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

	SC_HANDLE schSCManager = OpenSCManagerW(NULL,SERVICES_ACTIVE_DATABASEW,SC_MANAGER_CREATE_SERVICE);
	if (!schSCManager)
		LOG_ERROR_RETURN(("OpenSCManagerW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

  	SC_HANDLE schService = CreateServiceW( 
		schSCManager,                // SCManager database 
		SERVICE_NAME,                // name of service 
		L"Omega Online Network Hub", // service name to display 
		SERVICE_ALL_ACCESS,          // desired access 
		SERVICE_WIN32_OWN_PROCESS,   // service type 
		SERVICE_DEMAND_START,        // start type 
		SERVICE_ERROR_NORMAL,        // error control type 
		szPath,                      // path to service's binary 
		NULL,                        // no load ordering group 
		NULL,                        // no tag identifier 
		NULL,                        // no dependencies 
		NULL,                        // LocalSystem account 
		NULL);                       // no password 

	DWORD dwErr = GetLastError();
	if (!schService && dwErr != ERROR_SERVICE_EXISTS)
		LOG_ERROR(("CreateServiceW failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()));

	CloseServiceHandle(schSCManager);

	if (!schService)
		return (dwErr == ERROR_SERVICE_EXISTS);
	
	SERVICE_DESCRIPTIONW sdesc;
	sdesc.lpDescription = L"Manages the peer connections for the Omega Online network";
	ChangeServiceConfig2(schService,SERVICE_CONFIG_DESCRIPTION,&sdesc);
	
	CloseServiceHandle(schService);

	return true;
}

bool Root::Manager::platform_uninstall()
{
	SC_HANDLE schSCManager = OpenSCManagerW(NULL,SERVICES_ACTIVE_DATABASEW,SC_MANAGER_CREATE_SERVICE);
	if (!schSCManager)
		LOG_ERROR_RETURN(("OpenSCManagerW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

	SC_HANDLE schService = OpenServiceW(schSCManager,SERVICE_NAME,DELETE | SERVICE_QUERY_STATUS);
	if (!schService)
	{
		DWORD err = GetLastError();
		CloseServiceHandle(schSCManager);
		if (err == ERROR_SERVICE_DOES_NOT_EXIST)
			return true;

		LOG_ERROR_RETURN(("OpenServiceW failed: %s",OOBase::Win32::FormatMessage(err).c_str()),false);
	}

	// Get the current service status
	SERVICE_STATUS ss = {0};
	DWORD dwBytes = 0;
	if (!QueryServiceStatusEx(schService,SC_STATUS_PROCESS_INFO,(LPBYTE)&ss,sizeof(ss),&dwBytes))
	{
		LOG_ERROR(("QueryServiceStatusEx failed: %s",OOBase::Win32::FormatMessage().c_str()));
		CloseServiceHandle(schSCManager);
		CloseServiceHandle(schService);
		return false;
	}

	if (ss.dwCurrentState != SERVICE_STOPPED)
	{
		CloseServiceHandle(schSCManager);
		CloseServiceHandle(schService);
		std::cerr << "You must stop the OOServer service before uninstalling." << std::endl;
		return false;
	}
	
	if (!DeleteService(schService))
	{
		LOG_ERROR(("DeleteService failed: %s",OOBase::Win32::FormatMessage().c_str()));
		CloseServiceHandle(schSCManager);
		CloseServiceHandle(schService);
		return false;
	}

	return true;
}

bool Root::Manager::install_sandbox(int argc, char* argv[])
{
	std::wstring strUName = L"_OMEGA_SANDBOX_USER_";
	std::wstring strPwd = L"4th_(*%LGe895y^$N|2";

	if (argc>=1)
		strUName = OOBase::from_native(argv[0]);
	if (argc>=2)
		strPwd = OOBase::from_native(argv[1]);

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
		LOG_ERROR_RETURN(("NetUserAdd failed: %s",OOBase::Win32::FormatMessage(err).c_str()),false);

	// Now we have to add the SE_BATCH_LOGON_NAME priviledge and remove all the others

	// Lookup the account SID
	DWORD dwSidSize = 0;
	DWORD dwDnSize = 0;
	SID_NAME_USE use;
	if (!LookupAccountNameW(NULL,info.usri2_name,NULL,&dwSidSize,NULL,&dwDnSize,&use))
	{
		err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER)
		{
			if (bAddedUser)
				NetUserDel(NULL,info.usri2_name);
			LOG_ERROR_RETURN(("LookupAccountNameW failed: %s",OOBase::Win32::FormatMessage(err).c_str()),false);
		}
	}

	if (dwSidSize==0)
	{
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		LOG_ERROR_RETURN(("Added user has invalid SID"),false);
	}
	
	OOBase::SmartPtr<void,OOBase::FreeDestructor<void> > pSid = static_cast<PSID>(malloc(dwSidSize));
	if (!pSid)
	{
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		LOG_ERROR_RETURN(("Out of memeory"),false);
	}
	
	OOBase::SmartPtr<wchar_t,OOBase::ArrayDestructor<wchar_t> > pszDName;
	OOBASE_NEW(pszDName,wchar_t[dwDnSize]);
	if (!pszDName)
	{
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		LOG_ERROR_RETURN(("Out of memory"),false);
	}
	
	if (!LookupAccountNameW(NULL,info.usri2_name,pSid.value(),&dwSidSize,pszDName.value(),&dwDnSize,&use))
	{
		err = GetLastError();
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		LOG_ERROR_RETURN(("LookupAccountNameW failed: %s",OOBase::Win32::FormatMessage(err).c_str()),false);
	}
		
	if (use != SidTypeUser)
	{
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		LOG_ERROR_RETURN(("Added user has ended up of the wrong type"),false);
	}

	// Open the local account policy...
	LSA_HANDLE hPolicy;
	LSA_OBJECT_ATTRIBUTES oa = {0};
	NTSTATUS err2 = LsaOpenPolicy(NULL,&oa,POLICY_ALL_ACCESS,&hPolicy);
	if (err2 != 0)
	{
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		LOG_ERROR_RETURN(("LsaOpenPolicy failed: %s",OOBase::Win32::FormatMessage(err2).c_str()),false);
	}

	LSA_UNICODE_STRING szName;
	szName.Buffer = L"SeBatchLogonRight";
	size_t len = wcslen(szName.Buffer);
	szName.Length = static_cast<USHORT>(len * sizeof(WCHAR));
	szName.MaximumLength = static_cast<USHORT>((len+1) * sizeof(WCHAR));

	err2 = LsaAddAccountRights(hPolicy,pSid.value(),&szName,1);
	if (err2 == 0)
	{
		// Remove all other priviledges
		LSA_UNICODE_STRING* pRightsList;
		ULONG ulRightsCount = 0;
		err2 = LsaEnumerateAccountRights(hPolicy,pSid.value(),&pRightsList,&ulRightsCount);
		if (err2 == 0)
		{
			for (ULONG i=0;i<ulRightsCount;i++)
			{
				if (wcscmp(pRightsList[i].Buffer,L"SeBatchLogonRight") != 0)
				{
					err2 = LsaRemoveAccountRights(hPolicy,pSid.value(),FALSE,&pRightsList[i],1);
					if (err2 != 0)
						break;
				}
			}
		}
	}

	// Done with policy handle
	LsaClose(hPolicy);

	if (err2 != 0)
	{
		if (bAddedUser)
			NetUserDel(NULL,info.usri2_name);
		LOG_ERROR_RETURN(("Adjusting user priviledges failed: %s",OOBase::Win32::FormatMessage(err2).c_str()),false);
	}

	// Set the user name and pwd...
	Omega::int64_t key = 0;
	int err3 = m_registry->open_key(key,"Server\\Sandbox",0);
	if (err3 != 0)
		LOG_ERROR_RETURN(("Adding user information to registry failed: %s",OOBase::strerror(err3).c_str()),false);

	err3 = m_registry->set_string_value(key,"UserName",0,OOBase::to_utf8(info.usri2_name).c_str());
	if (err3 != 0)
		LOG_ERROR_RETURN(("Adding user information to registry failed: %s",OOBase::strerror(err3).c_str()),false);

	err3 = m_registry->set_string_value(key,"Password",0,OOBase::to_utf8(info.usri2_password).c_str());
	if (err3 != 0)
		LOG_ERROR_RETURN(("Adding user information to registry failed: %s",OOBase::strerror(err3).c_str()),false);

	if (bAddedUser)
		m_registry->set_integer_value(key,"AutoAdded",0,1);

	return true;
}

bool Root::Manager::uninstall_sandbox()
{
	if (!init_database())
		return false;

	// Get the user name and pwd...
	Omega::int64_t key = 0;
	if (m_registry->open_key(key,"Server\\Sandbox",0) != 0)
		return true;

	std::string strUName;
	if (m_registry->get_string_value(key,"UserName",0,strUName) == 0)
	{
		Omega::int64_t bUserAdded = 0;
		m_registry->get_integer_value(key,"AutoAdded",0,bUserAdded);
		if (bUserAdded == 1)
		{
			NET_API_STATUS err = NetUserDel(NULL,OOBase::from_utf8(strUName.c_str()).c_str());
			if (err != NERR_Success)
				LOG_ERROR_RETURN(("NetUserDel failed: %s",OOBase::Win32::FormatMessage(err).c_str()),false);
		}
	}

	return true;
}

bool Root::Manager::secure_file(const std::string& strFile, bool bPublicRead)
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
		ea[0].grfAccessPermissions = GENERIC_READ;
		ea[0].grfAccessMode = SET_ACCESS;
		ea[0].grfInheritance = NO_INHERITANCE;
		ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea[0].Trustee.ptstrName = (LPWSTR)pSIDUsers.value();
	}
	else
	{
		ea[0].grfAccessPermissions = 0;//GENERIC_READ;
		ea[0].grfAccessMode = REVOKE_ACCESS;
		ea[0].grfInheritance = NO_INHERITANCE;
		ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea[0].Trustee.ptstrName = (LPWSTR)pSIDUsers.value();
	}

	// Set full control for Administrators.
	ea[1].grfAccessPermissions = GENERIC_ALL;
	ea[1].grfAccessMode = GRANT_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[1].Trustee.ptstrName = (LPWSTR)pSIDAdmin.value();

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
}

bool Root::Manager::get_db_directory(std::string& dir)
{
	wchar_t szBuf[MAX_PATH] = {0};
	HRESULT hr = SHGetFolderPathW(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
	if FAILED(hr)
		LOG_ERROR_RETURN(("SHGetFolderPathW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);
	
	if (!PathAppendW(szBuf,L"Omega Online"))
		LOG_ERROR_RETURN(("PathAppendW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);
	
	if (!PathFileExistsW(szBuf))
	{
		if (!CreateDirectoryW(szBuf,NULL))
			LOG_ERROR_RETURN(("CreateDirectoryW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);
	}

	dir = OOBase::to_utf8(szBuf);
	if (*dir.rbegin() != '\\')
		dir += '\\';

	return true;
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

void Root::Manager::wait_for_quit()
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
}

namespace OOBase
{
	// This is the critical failure hook
	void CriticalFailure(const char* msg)
	{
		OOSvrBase::Logger::log(OOSvrBase::Logger::Error,msg);

		// Die horribly now!
		TerminateProcess(GetCurrentProcess(),EXIT_FAILURE);
	}
}

#endif // _WIN32
