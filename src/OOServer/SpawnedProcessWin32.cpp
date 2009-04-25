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
#include "SpawnedProcess.h"

#if defined(_WIN32)

#include "../OOBase/SecurityWin32.h"
#include "../OOBase/Win32Socket.h"
#include "../OOBase/SmartPtr.h"

#include <sddl.h>
#include <shlwapi.h>
#include <shlobj.h>

void AttachDebugger(DWORD pid);

namespace
{
	class SpawnedProcessWin32 : public Root::SpawnedProcess
	{
	public:
		SpawnedProcessWin32();
		virtual ~SpawnedProcessWin32();

		bool Spawn(bool bUnsafe, HANDLE hToken, const std::string& strPipe, bool bSandbox);
		bool CheckAccess(const char* pszFName, int mode, bool& bAllowed);
		bool Compare(OOBase::LocalSocket::uid_t uid);
		bool IsSameUser(OOBase::LocalSocket::uid_t uid);
		std::string GetRegistryHive();

		static Root::SpawnedProcess* Spawn(OOBase::LocalSocket::uid_t uid);
				
	private:
		bool                       m_bSandbox;
		OOBase::Win32::SmartHandle m_hToken;
		OOBase::Win32::SmartHandle m_hProcess;
		HANDLE                     m_hProfile;

		DWORD SpawnFromToken(HANDLE hToken, const std::string& strPipe, bool bSandbox);
	};
}

namespace
{
	static HANDLE CreatePipe(HANDLE hToken, std::string& strPipe)
	{
		// Create a new unique pipe
		strPipe = "OOR";

		// Get the logon SID of the Token
		OOBase::SmartPtr<void,OOBase::FreeDestructor<void> > ptrSIDLogon = 0;
		DWORD dwRes = OOSvrBase::Win32::GetLogonSID(hToken,ptrSIDLogon);
		if (dwRes != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("GetLogonSID failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),INVALID_HANDLE_VALUE);
		
		char* pszSid;
		if (ConvertSidToStringSidA(ptrSIDLogon.value(),&pszSid))
		{
			strPipe += pszSid;
			LocalFree(pszSid);
		}
				
		char szBuf[64];
		OOBase::timeval_t now = OOBase::gettimeofday();
		sprintf_s(szBuf,sizeof(szBuf),"-%lx",now.tv_usec);
		strPipe += szBuf;

		// Get the current processes user SID
		OOBase::Win32::SmartHandle hProcessToken;
		if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
			LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::Win32::FormatMessage().c_str()),INVALID_HANDLE_VALUE);
			
		OOBase::SmartPtr<TOKEN_USER,OOBase::FreeDestructor<TOKEN_USER> > ptrSIDProcess = static_cast<TOKEN_USER*>(OOSvrBase::Win32::GetTokenInfo(hProcessToken,TokenUser));
		if (!ptrSIDProcess)
			LOG_ERROR_RETURN(("GetTokenInfo failed: %s",OOBase::Win32::FormatMessage().c_str()),INVALID_HANDLE_VALUE);
					
		// Create security descriptor
		static const int NUM_ACES = 2;
		EXPLICIT_ACCESSW ea[NUM_ACES] = {0};
		
		// Set full control for the calling process SID
		ea[0].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
		ea[0].grfAccessMode = SET_ACCESS;
		ea[0].grfInheritance = NO_INHERITANCE;
		ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea[0].Trustee.ptstrName = (LPWSTR)ptrSIDProcess->User.Sid;
		
		// Set full control for Specific user.
		ea[1].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
		ea[1].grfAccessMode = SET_ACCESS;
		ea[1].grfInheritance = NO_INHERITANCE;
		ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[1].Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea[1].Trustee.ptstrName = (LPWSTR)ptrSIDLogon.value();

		OOSvrBase::Win32::sec_descript_t sd;
		dwRes = sd.SetEntriesInAcl(NUM_ACES,ea,NULL);
		if (dwRes != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("SetEntriesInAcl failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),INVALID_HANDLE_VALUE);

		// Create security attribute
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = FALSE;
		sa.lpSecurityDescriptor = sd.descriptor();

		// Create the named pipe instance
		HANDLE hPipe = CreateNamedPipeA(("\\\\.\\pipe\\" + strPipe).c_str(),
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
			PIPE_TYPE_BYTE |
			PIPE_READMODE_BYTE |
			PIPE_WAIT,
			1,
			0,
			0,
			0,
			&sa);

		if (hPipe == INVALID_HANDLE_VALUE)
			LOG_ERROR(("CreateNamedPipeA failed: %s",OOBase::Win32::FormatMessage().c_str()));
			
		return hPipe;
	}

	static bool WaitForConnect(HANDLE hPipe)
	{
		OVERLAPPED ov = {0};
		ov.hEvent = CreateEventW(NULL,TRUE,TRUE,NULL);
		if (!ov.hEvent)
			OOBase_CallCriticalFailure(GetLastError());

		// Control handle lifetime
		OOBase::Win32::SmartHandle ev(ov.hEvent);

		DWORD dwErr = 0;
		if (ConnectNamedPipe(hPipe,&ov))
			dwErr = ERROR_PIPE_CONNECTED;
		else
		{
			dwErr = GetLastError();
			if (dwErr == ERROR_IO_PENDING)
				dwErr = 0;
		}

		if (dwErr == ERROR_PIPE_CONNECTED)
		{
			dwErr = 0;
			if (!SetEvent(ov.hEvent))
				OOBase_CallCriticalFailure(GetLastError());
		}

		if (dwErr != 0)
			LOG_ERROR(("ConnectNamedPipe failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()));
		else
		{
			DWORD dw = 0;
			if (!GetOverlappedResult(hPipe,&ov,&dw,TRUE))
			{
				dwErr = GetLastError();
				LOG_ERROR(("GetOverlappedResult failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()));
			}
		}

		return (dwErr == 0);
	}

	static bool LogonSandboxUser(const std::wstring& strUName, const std::wstring& strPwd, HANDLE& hToken)
	{
		if (!LogonUserW((LPWSTR)strUName.c_str(),NULL,(LPWSTR)strPwd.c_str(),LOGON32_LOGON_BATCH,LOGON32_PROVIDER_DEFAULT,&hToken))
			LOG_ERROR_RETURN(("LogonUserW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);
		
		// Control handle lifetime
		OOBase::Win32::SmartHandle tok(hToken);

		// Restrict the Token
		DWORD dwErr = OOSvrBase::Win32::RestrictToken(hToken);
		if (dwErr != 0)
			LOG_ERROR_RETURN(("RestrictToken failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),false);

		// This might be needed for retricted tokens...
		dwErr = OOSvrBase::Win32::SetTokenDefaultDACL(hToken);
		if (dwErr != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("SetTokenDefaultDACL failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),false);

		tok.detach();
		return true;
	}

	static DWORD CreateWindowStationSD(TOKEN_USER* pProcessUser, PSID pSIDLogon, OOSvrBase::Win32::sec_descript_t& sd)
	{
		const int NUM_ACES = 3;
		EXPLICIT_ACCESSW ea[NUM_ACES] = {0};
		
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

		return sd.SetEntriesInAcl(NUM_ACES,ea,NULL);
	}

	static DWORD CreateDesktopSD(TOKEN_USER* pProcessUser, PSID pSIDLogon, OOSvrBase::Win32::sec_descript_t& sd)
	{
		const int NUM_ACES = 2;
		EXPLICIT_ACCESSW ea[NUM_ACES] = {0};
		
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

		return sd.SetEntriesInAcl(NUM_ACES,ea,NULL);
	}

	static DWORD OpenCorrectWindowStation(HANDLE hToken, std::wstring& strWindowStation, HWINSTA& hWinsta, HDESK& hDesktop)
	{
		// Service window stations are created with the name "Service-0xZ1-Z2$",
		// where Z1 is the high part of the logon SID and Z2 is the low part of the logon SID
		// see http://msdn2.microsoft.com/en-us/library/ms687105.aspx for details

		// Get the logon SID of the Token
		OOBase::SmartPtr<void,OOBase::FreeDestructor<void> > ptrSIDLogon = 0;
		DWORD dwRes = OOSvrBase::Win32::GetLogonSID(hToken,ptrSIDLogon);
		if (dwRes != ERROR_SUCCESS)
			return dwRes;

		wchar_t* pszSid = 0;
		if (!ConvertSidToStringSidW(ptrSIDLogon.value(),&pszSid))
			return GetLastError();
			
		strWindowStation = pszSid;
		LocalFree(pszSid);
		
		// Logon SIDs are of the form S-1-5-5-X-Y, and we want X and Y
		if (wcsncmp(strWindowStation.c_str(),L"S-1-5-5-",8) != 0)
			return ERROR_INVALID_SID;
		
		// Crack out the last two parts - there is probably an easier way... but this works
		strWindowStation = strWindowStation.substr(8);
		const wchar_t* p = strWindowStation.c_str();
		wchar_t* pEnd = 0;
		DWORD dwParts[2];
		dwParts[0] = wcstoul(p,&pEnd,10);
		if (*pEnd != L'-')
			return ERROR_INVALID_SID;
		
		dwParts[1] = wcstoul(pEnd+1,&pEnd,10);
		if (*pEnd != L'\0')
			return ERROR_INVALID_SID;
		
		wchar_t szBuf[256];
		swprintf_s(szBuf,sizeof(szBuf),L"Service-0x%lx-%lx$",dwParts[0],dwParts[1]);
	
		strWindowStation = szBuf;

		// Get the current processes user SID
		OOBase::Win32::SmartHandle hProcessToken;
		if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
			return GetLastError();
			
		OOBase::SmartPtr<TOKEN_USER,OOBase::FreeDestructor<TOKEN_USER> > ptrProcessUser = static_cast<TOKEN_USER*>(OOSvrBase::Win32::GetTokenInfo(hProcessToken,TokenUser));
		if (!ptrProcessUser)
			return GetLastError();

		// Open or create the new window station and desktop
		// Now confirm the Window Station exists...
		hWinsta = OpenWindowStationW((LPWSTR)strWindowStation.c_str(),FALSE,WINSTA_CREATEDESKTOP);
		if (!hWinsta)
		{
			// See if just doesn't exist yet...
			dwRes = GetLastError();
			if (dwRes != ERROR_FILE_NOT_FOUND)
				return dwRes;
			
			OOSvrBase::Win32::sec_descript_t sd;
			dwRes = CreateWindowStationSD(ptrProcessUser.value(),ptrSIDLogon.value(),sd);
			if (dwRes != ERROR_SUCCESS)
				return dwRes;

			SECURITY_ATTRIBUTES sa;
			sa.nLength = sizeof(sa);
			sa.bInheritHandle = FALSE;
			sa.lpSecurityDescriptor = sd.descriptor();
			
			hWinsta = CreateWindowStationW(strWindowStation.c_str(),0,WINSTA_CREATEDESKTOP,&sa);
			if (!hWinsta)
				return GetLastError();
		}

		// Stash our old
		HWINSTA hOldWinsta = GetProcessWindowStation();
		if (!hOldWinsta)
		{
			dwRes = GetLastError();
			CloseWindowStation(hWinsta);
			return dwRes;
		}

		// Swap our Window Station
		if (!SetProcessWindowStation(hWinsta))
		{
			dwRes = GetLastError();
			CloseWindowStation(hWinsta);
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
				SetProcessWindowStation(hOldWinsta);
				CloseWindowStation(hWinsta);
				return dwRes;
			}

			OOSvrBase::Win32::sec_descript_t sd;
			dwRes = CreateDesktopSD(ptrProcessUser.value(),ptrSIDLogon.value(),sd);
			if (dwRes != ERROR_SUCCESS)
			{
				SetProcessWindowStation(hOldWinsta);
				CloseWindowStation(hWinsta);
				return dwRes;
			}

			SECURITY_ATTRIBUTES sa;
			sa.nLength = sizeof(sa);
			sa.bInheritHandle = FALSE;
			sa.lpSecurityDescriptor = sd.descriptor();

			hDesktop = CreateDesktopW(L"default",0,0,0,DESKTOP_CREATEWINDOW,&sa);
			if (!hDesktop)
			{
				dwRes = GetLastError();
				SetProcessWindowStation(hOldWinsta);
				CloseWindowStation(hWinsta);
				return dwRes;
			}
		}

		// Revert our Window Station
		SetProcessWindowStation(hOldWinsta);

		strWindowStation += L"\\default";

		return ERROR_SUCCESS;
	}
}

SpawnedProcessWin32::SpawnedProcessWin32() :
	m_hToken(NULL),
	m_hProcess(NULL),
	m_hProfile(NULL)	
{
}

SpawnedProcessWin32::~SpawnedProcessWin32()
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
	}

	if (m_hProfile)
		UnloadUserProfile(m_hToken,m_hProfile);
}

DWORD SpawnedProcessWin32::SpawnFromToken(HANDLE hToken, const std::string& strPipe, bool bSandbox)
{
	// Get our module name
	wchar_t szPath[MAX_PATH];
	if (!GetModuleFileNameW(NULL,szPath,MAX_PATH))
		return GetLastError();
	
	// Strip off our name, and add OOSvrUser.exe
	if (!PathRemoveFileSpecW(szPath))
		return GetLastError();

	if (!PathAppendW(szPath,L"OOSvrUser.exe"))
		return GetLastError();

	std::wstring strCmdLine = L"\"";
	strCmdLine += szPath;
	strCmdLine += L"\" ";
	strCmdLine += OOBase::from_native(strPipe.c_str());

	// Forward declare these because of goto's
	STARTUPINFOW startup_info = {0};
	std::wstring strWindowStation;
	HWINSTA hWinsta = 0;
	HDESK hDesktop = 0;
	DWORD dwFlags = CREATE_UNICODE_ENVIRONMENT | CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_PROCESS_GROUP;
	HANDLE hDebugEvent = NULL;
	HANDLE hPriToken = 0;
	std::wstring strTitle;
	bool bPrivError = false;

	// Load up the users profile
	HANDLE hProfile = NULL;
	DWORD dwRes = 0;
	if (!bSandbox)
	{
		dwRes = OOSvrBase::Win32::LoadUserProfileFromToken(hToken,hProfile);
		if (dwRes != ERROR_SUCCESS)
			return dwRes;
	}

	// Load the users environment vars
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
	startup_info.cb = sizeof(STARTUPINFOW);
	startup_info.dwFlags = STARTF_USESHOWWINDOW;
	startup_info.wShowWindow = SW_MINIMIZE;

#if defined(OMEGA_DEBUG)
	if (IsDebuggerPresent())
	{
		hDebugEvent = CreateEventW(NULL,FALSE,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX");
		
		dwFlags |= CREATE_NEW_CONSOLE;

		strTitle = szPath;

		// Get the names associated with the user SID
        std::wstring strUserName;
        std::wstring strDomainName;
        if (OOSvrBase::Win32::GetNameFromToken(hPriToken,strUserName,strDomainName) == ERROR_SUCCESS)
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
	if (!CreateProcessAsUserW(hPriToken,NULL,(wchar_t*)strCmdLine.c_str(),NULL,NULL,FALSE,dwFlags,lpEnv,NULL,&startup_info,&process_info))
	{
		dwRes = GetLastError();
		if (dwRes == ERROR_PRIVILEGE_NOT_HELD)
			bPrivError = true;

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

	return dwRes;
}

bool SpawnedProcessWin32::Spawn(bool bUnsafe, HANDLE hToken, const std::string& strPipe, bool bSandbox)
{
	m_bSandbox = bSandbox;

	DWORD dwRes = SpawnFromToken(hToken,strPipe,bSandbox);
	if (dwRes != ERROR_SUCCESS)
	{
		if (dwRes == ERROR_PRIVILEGE_NOT_HELD && (bUnsafe || IsDebuggerPresent()))
		{
			OOBase::Win32::SmartHandle hToken2;
			if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,&hToken2))
				OOBase_CallCriticalFailure(GetLastError());
			else
			{
				// Get the names associated with the user SID
				std::wstring strUserName;
				std::wstring strDomainName;

				if (OOSvrBase::Win32::GetNameFromToken(hToken2,strUserName,strDomainName) == ERROR_SUCCESS)
				{
					std::wstring strMsg =
						L"OOServer is running under a user account that does not have the priviledges required to spawn processes as a different user.\n\n"
						L"Because the 'Unsafe' value is set in the registry, or a debugger is attached to OOServer, the new user process will be started under the user account '";
					
					strMsg += strDomainName;
					strMsg += L"\\";
					strMsg += strUserName;
					strMsg += L"'\n\nThis is a security risk, and should only be allowed for debugging purposes, and only then if you really know what you are doing.";

					std::wstring strMsg2 = strMsg;
					strMsg2 += L"\n\nDo you want to allow this?";

					if (MessageBoxW(NULL,strMsg2.c_str(),L"OOServer - Important security warning",MB_ICONEXCLAMATION | MB_YESNO | MB_SERVICE_NOTIFICATION | MB_DEFAULT_DESKTOP_ONLY | MB_DEFBUTTON2) == IDYES)
					{
						// Restrict the Token
						dwRes = OOSvrBase::Win32::RestrictToken(hToken2);
						if (dwRes == ERROR_SUCCESS)
						{
							dwRes = SpawnFromToken(hToken2,strPipe,bSandbox);
							if (dwRes == ERROR_SUCCESS)
							{
								OOSvrBase::Logger::log(OOSvrBase::Logger::Warning,"%ls",strMsg.c_str());
								
								CloseHandle(m_hToken);
								DuplicateToken(hToken2,SecurityImpersonation,&m_hToken);
							}
						}
					}
				}
			}
		}

		if (dwRes != ERROR_SUCCESS)
		{
			// We really need to bitch about this now...
			LOG_ERROR(("SpawnFromToken failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()));
		}
	}

	return (dwRes == ERROR_SUCCESS);
}

bool SpawnedProcessWin32::CheckAccess(const char* pszFName, int mode, bool& bAllowed)
{
	bAllowed = false;

	OOBase::SmartPtr<void,OOBase::FreeDestructor<void> > pSD = 0;
	DWORD cbNeeded = 0;
	if (!GetFileSecurityA(pszFName,DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,(PSECURITY_DESCRIPTOR)pSD.value(),0,&cbNeeded) && GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
		LOG_ERROR_RETURN(("GetFileSecurityA failed: %s",OOBase::Win32::FormatMessage().c_str()),false);
	
	pSD = malloc(cbNeeded);
	if (!pSD)
		LOG_ERROR_RETURN(("Out of memory"),false);

	if (!GetFileSecurityA(pszFName,DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,(PSECURITY_DESCRIPTOR)pSD.value(),cbNeeded,&cbNeeded))
		LOG_ERROR_RETURN(("GetFileSecurityA failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

	// Map the generic access rights
	DWORD dwAccessDesired = 0;
	if (mode & _O_RDONLY)
		dwAccessDesired = FILE_GENERIC_READ;
	else if (mode & _O_WRONLY)
		dwAccessDesired = FILE_GENERIC_WRITE;
	else if (mode & _O_RDWR)
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
	BOOL bRes = ::AccessCheck((PSECURITY_DESCRIPTOR)pSD.value(),m_hToken,dwAccessDesired,&generic,&privilege_set,&dwPrivSetSize,&dwAccessGranted,&bAllowedVal);
	DWORD err = GetLastError();
	
	if (!bRes && err != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("AccessCheck failed: %s",OOBase::Win32::FormatMessage(err).c_str()),false);
	
	bAllowed = (bAllowedVal == TRUE);
	return true;
}

bool SpawnedProcessWin32::Compare(HANDLE hToken)
{
	// Check the SIDs and priviledges are the same...
	OOBase::SmartPtr<TOKEN_GROUPS_AND_PRIVILEGES,OOBase::FreeDestructor<TOKEN_GROUPS_AND_PRIVILEGES> > pStats1 = static_cast<TOKEN_GROUPS_AND_PRIVILEGES*>(OOSvrBase::Win32::GetTokenInfo(hToken,TokenGroupsAndPrivileges));
	if (!pStats1)
		return false;

	OOBase::SmartPtr<TOKEN_GROUPS_AND_PRIVILEGES,OOBase::FreeDestructor<TOKEN_GROUPS_AND_PRIVILEGES> > pStats2 = static_cast<TOKEN_GROUPS_AND_PRIVILEGES*>(OOSvrBase::Win32::GetTokenInfo(m_hToken,TokenGroupsAndPrivileges));
	if (!pStats2)
		return false;
	
	return (pStats1->SidCount==pStats2->SidCount &&
		pStats1->RestrictedSidCount==pStats2->RestrictedSidCount &&
		pStats1->PrivilegeCount==pStats2->PrivilegeCount &&
		OOSvrBase::Win32::MatchSids(pStats1->SidCount,pStats1->Sids,pStats2->Sids) &&
		OOSvrBase::Win32::MatchSids(pStats1->RestrictedSidCount,pStats1->RestrictedSids,pStats2->RestrictedSids) &&
		OOSvrBase::Win32::MatchPrivileges(pStats1->PrivilegeCount,pStats1->Privileges,pStats2->Privileges));
}

bool SpawnedProcessWin32::IsSameUser(HANDLE hToken)
{
	OOBase::SmartPtr<TOKEN_USER,OOBase::FreeDestructor<TOKEN_USER> > ptrUserInfo1 = static_cast<TOKEN_USER*>(OOSvrBase::Win32::GetTokenInfo(hToken,TokenUser));
	if (!ptrUserInfo1)
		return false;

	OOBase::SmartPtr<TOKEN_USER,OOBase::FreeDestructor<TOKEN_USER> > ptrUserInfo2 = static_cast<TOKEN_USER*>(OOSvrBase::Win32::GetTokenInfo(m_hToken,TokenUser));
	if (!ptrUserInfo2)
		return false;
	
	return (EqualSid(ptrUserInfo1->User.Sid,ptrUserInfo2->User.Sid) == TRUE);
}

std::string SpawnedProcessWin32::GetRegistryHive()
{
	wchar_t szBuf[MAX_PATH] = {0};
	HRESULT hr;
	if (m_bSandbox)
		hr = SHGetFolderPathW(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
	else
		hr = SHGetFolderPathW(0,CSIDL_LOCAL_APPDATA,m_hToken,SHGFP_TYPE_DEFAULT,szBuf);

	if SUCCEEDED(hr)
	{
		wchar_t szBuf2[MAX_PATH] = {0};
		if (PathCombineW(szBuf2,szBuf,L"Omega Online"))
		{
			if (PathFileExistsW(szBuf2) || CreateDirectoryW(szBuf2,NULL) == 0)
			{
				if (PathCombineW(szBuf,szBuf2,L"user.regdb"))
					return OOBase::to_utf8(szBuf);
			}
		}
	}

	return std::string();
}

OOBase::SmartPtr<Root::SpawnedProcess> Root::Manager::platform_spawn(OOBase::LocalSocket::uid_t uid, std::string& strPipe, Omega::uint32_t& channel_id, OOBase::SmartPtr<MessageConnection>& ptrMC)
{
	// Stash the sandbox flag because we adjust uid...
	bool bSandbox = (uid == OOBase::LocalSocket::uid_t(-1));
	OOBase::Win32::SmartHandle sandbox_uid;
	if (bSandbox)
	{
		// Get the user name and pwd...
		Omega::int64_t key = 0;
		if (m_registry->open_key(key,"Server\\Sandbox",0) != 0)
			return 0;

		std::string strUName;
		std::string strPwd;
		int err = m_registry->get_string_value(key,"UserName",0,strUName);
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to read sandbox username from registry: %s",OOSvrBase::Logger::strerror(err).c_str()),(SpawnedProcess*)0);

		m_registry->get_string_value(key,"Password",0,strPwd);
		
		if (!LogonSandboxUser(OOBase::from_utf8(strUName.c_str()),OOBase::from_utf8(strPwd.c_str()),uid))
			return 0;

		// Make sure it's closed
		sandbox_uid = uid;
	}

	// Work out if we are running in unsafe mode
	Omega::int64_t key = 0;
	Omega::int64_t v = 0;
	if (bSandbox && m_registry->open_key(key,"Server\\Sandbox",0) == 0)
		m_registry->get_integer_value(key,"Unsafe",0,v);

	bool bUnsafe = (v == 1);

	// Alloc a new SpawnedProcess
	SpawnedProcessWin32* pSpawn32 = 0;
	OOBASE_NEW(pSpawn32,SpawnedProcessWin32);
	if (!pSpawn32)
		return 0;

	// Create the named pipe
	OOBase::Win32::SmartHandle hPipe(CreatePipe(uid,strPipe));
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		delete pSpawn32;
		return 0;
	}
		
	// Spawn the process
	if (!pSpawn32->Spawn(bUnsafe,uid,strPipe,bSandbox))
	{
		delete pSpawn32;
		return 0;
	}

	OOBase::SmartPtr<Root::SpawnedProcess> pSpawn = pSpawn32;
	
	// Wait for the connect attempt
	if (!WaitForConnect(hPipe))
		return 0;
	
	// Now wrap the handle in a local socket
	OOBase::Win32::SmartHandle hReadEvent(CreateEventW(NULL,TRUE,FALSE,NULL));
	if (!hReadEvent)
		return 0;
		
	OOBase::Win32::SmartHandle hWriteEvent(CreateEventW(NULL,TRUE,FALSE,NULL));
	if (!hWriteEvent)
		return 0;
		
	OOBase::Win32::Socket sock(hPipe.detach(),hReadEvent.detach(),hWriteEvent.detach());

	// Bootstrap the user process...
	channel_id = bootstrap_user(&sock,ptrMC,strPipe);
	if (!channel_id)
		return 0;

	CREATE_IPC_SOCKET_HERE!
	
	// Create an async socket wrapper
	int err = 0;
	OOSvrBase::AsyncSocket* pAsync = Proactor::instance()->attach_socket(ptrMC.value(),&err,(OOBase::Win32::Socket*)&sock);
	if (err != 0)
		return 0;

	// Attach the async socket to the message connection
	ptrMC->attach(pAsync);

	return pSpawn;
}

#endif // _WIN32
