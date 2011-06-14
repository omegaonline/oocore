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
#include "RootManager.h"
#include "SpawnedProcess.h"

#if defined(_WIN32)

#include <sddl.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <ntsecapi.h>

void AttachDebugger(DWORD pid);

namespace
{
	int getenv_i(const char* val, OOBase::LocalString& str)
	{
		int ret = 0;

	#if defined(_MSC_VER) && defined(_CRT_INSECURE_DEPRECATE)
		char* buf = 0;
		size_t len = 0;
		if (!_dupenv_s(&buf,&len,val))
		{
			if (len)
				ret = str.assign(buf,len-1);
			free(buf);
		}
	#else
		ret = str.assign(getenv(val));
	#endif

		return ret;
	}

	bool getenv_OMEGA_DEBUG()
	{
		OOBase::LocalString str;
		if (getenv_i("OMEGA_DEBUG",str) != 0)
			return false;

		return (str == "yes");
	}

	class SpawnedProcessWin32 : public Root::SpawnedProcess
	{
	public:
		SpawnedProcessWin32();
		virtual ~SpawnedProcessWin32();

		bool IsRunning() const;
		bool Spawn(const OOBase::LocalString& strAppPath, HANDLE hToken, OOBase::Win32::SmartHandle& hPipe, bool bSandbox, bool& bAgain);
		bool CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed) const;
		bool IsSameLogin(OOSvrBase::AsyncLocalSocket::uid_t uid) const;
		bool IsSameUser(OOSvrBase::AsyncLocalSocket::uid_t uid) const;
		bool GetRegistryHive(OOBase::String& strSysDir, OOBase::String& strUsersDir, OOBase::LocalString& strHive);

	private:
		bool                       m_bSandbox;
		OOBase::Win32::SmartHandle m_hToken;
		OOBase::Win32::SmartHandle m_hProcess;
		HANDLE                     m_hProfile;

		DWORD SpawnFromToken(const OOBase::LocalString& strAppPath, HANDLE hToken, OOBase::Win32::SmartHandle& hPipe, bool bSandbox);
	};

	static HANDLE CreatePipe(HANDLE hToken, OOBase::LocalString& strPipe)
	{
		// Create a new unique pipe

		// Get the logon SID of the Token
		OOBase::SmartPtr<void,OOBase::LocalDestructor> ptrSIDLogon;
		DWORD dwRes = OOSvrBase::Win32::GetLogonSID(hToken,ptrSIDLogon);
		if (dwRes != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("GetLogonSID failed: %s",OOBase::system_error_text(dwRes)),INVALID_HANDLE_VALUE);

		char* pszSid = NULL;
		if (!ConvertSidToStringSidA(ptrSIDLogon,&pszSid))
			LOG_ERROR_RETURN(("ConvertSidToStringSidA failed: %s",OOBase::system_error_text()),INVALID_HANDLE_VALUE);

		int err = strPipe.printf("OOR%s-%u-%u",pszSid,GetCurrentProcessId(),GetTickCount());
		::LocalFree(pszSid);

		if (err != 0)
			LOG_ERROR_RETURN(("Failed to compose pipe name: %s",OOBase::system_error_text(err)),INVALID_HANDLE_VALUE);

		// Create security descriptor
		PSID pSID;
		SID_IDENTIFIER_AUTHORITY SIDAuthCreator = {SECURITY_CREATOR_SID_AUTHORITY};
		if (!AllocateAndInitializeSid(&SIDAuthCreator, 1,
									  SECURITY_CREATOR_OWNER_RID,
									  0, 0, 0, 0, 0, 0, 0,
									  &pSID))
		{
			LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::system_error_text()),INVALID_HANDLE_VALUE);
		}
		OOBase::SmartPtr<void,OOSvrBase::Win32::SIDDestructor<void> > pSIDOwner(pSID);

		static const int NUM_ACES = 2;
		EXPLICIT_ACCESSW ea[NUM_ACES] = { {0}, {0} };

		// Set full control for the creating process SID
		ea[0].grfAccessPermissions = FILE_ALL_ACCESS;
		ea[0].grfAccessMode = SET_ACCESS;
		ea[0].grfInheritance = NO_INHERITANCE;
		ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea[0].Trustee.ptstrName = (LPWSTR)pSIDOwner;

		// Set read/write control for Specific logon.
		ea[1].grfAccessPermissions = FILE_GENERIC_READ | FILE_WRITE_DATA;
		ea[1].grfAccessMode = SET_ACCESS;
		ea[1].grfInheritance = NO_INHERITANCE;
		ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[1].Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea[1].Trustee.ptstrName = (LPWSTR)ptrSIDLogon;

		OOSvrBase::Win32::sec_descript_t sd;
		dwRes = sd.SetEntriesInAcl(NUM_ACES,ea,NULL);
		if (dwRes != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("SetEntriesInAcl failed: %s",OOBase::system_error_text(dwRes)),INVALID_HANDLE_VALUE);

		// Create security attribute
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = FALSE;
		sa.lpSecurityDescriptor = sd.descriptor();

		// Create the named pipe instance
		OOBase::LocalString strFullPipe;
		err = strFullPipe.concat("\\\\.\\pipe\\",strPipe.c_str());
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to concat strings: %s",OOBase::system_error_text(err)),INVALID_HANDLE_VALUE);

		HANDLE hPipe = CreateNamedPipeA(strFullPipe.c_str(),
										PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
										PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
										1,
										0,
										0,
										0,
										&sa);

		if (hPipe == INVALID_HANDLE_VALUE)
			LOG_ERROR(("CreateNamedPipeA failed: %s",OOBase::system_error_text()));

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
			LOG_ERROR(("ConnectNamedPipe failed: %s",OOBase::system_error_text(dwErr)));
		else
		{
			DWORD dw = 0;
			if (!GetOverlappedResult(hPipe,&ov,&dw,TRUE))
				LOG_ERROR(("GetOverlappedResult failed: %s",OOBase::system_error_text()));
		}

		if (dwErr != 0)
			LOG_ERROR(("WaitForConnect failed: %s",OOBase::system_error_text(dwErr)));

		return (dwErr == 0);
	}

	static DWORD LogonSandboxUser(const OOBase::String& strUName, HANDLE& hToken)
	{
		// Convert UName to wide
		size_t wlen = OOBase::from_native(NULL,0,strUName.c_str());
		OOBase::SmartPtr<wchar_t,OOBase::LocalDestructor> ptrUName = static_cast<wchar_t*>(OOBase::LocalAllocate(wlen*sizeof(wchar_t)));
		if (!ptrUName)
			LOG_ERROR_RETURN(("Out of memory"),ERROR_OUTOFMEMORY);

		OOBase::from_native(ptrUName,wlen,strUName.c_str());

		// Open the local account policy...
		LSA_HANDLE hPolicy;
		LSA_OBJECT_ATTRIBUTES oa = {0};
		DWORD dwErr = LsaNtStatusToWinError(LsaOpenPolicy(NULL,&oa,POLICY_GET_PRIVATE_INFORMATION,&hPolicy));
		if (dwErr != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("LsaOpenPolicy failed: %s",OOBase::system_error_text(dwErr)),dwErr);

		LSA_UNICODE_STRING szKey;
		szKey.Buffer = const_cast<PWSTR>(L"L$OOServer_sandbox_pwd");
		size_t len = wcslen(szKey.Buffer);
		szKey.Length = static_cast<USHORT>(len * sizeof(wchar_t));
		szKey.MaximumLength = static_cast<USHORT>((len+1) * sizeof(wchar_t));

		PLSA_UNICODE_STRING pszVal;
		dwErr = LsaNtStatusToWinError(LsaRetrievePrivateData(hPolicy,&szKey,&pszVal));
		if (dwErr != ERROR_SUCCESS)
		{
			LsaClose(hPolicy);
			LOG_ERROR_RETURN(("LsaRetrievePrivateData failed: %s",OOBase::system_error_text(dwErr)),dwErr);
		}

		OOBase::SmartPtr<wchar_t,OOBase::LocalDestructor> ptrPwd = static_cast<wchar_t*>(OOBase::LocalAllocate(pszVal->Length + sizeof(wchar_t)));
		if (ptrPwd)
		{
			memcpy(ptrPwd,pszVal->Buffer,pszVal->Length);
			ptrPwd[pszVal->Length/sizeof(wchar_t)] = L'\0';
		}

		BOOL bRes = LogonUserW(ptrUName,NULL,ptrPwd,LOGON32_LOGON_BATCH,LOGON32_PROVIDER_DEFAULT,&hToken);
		if (!bRes)
			dwErr = GetLastError();

#if defined(_MSC_VER)
		SecureZeroMemory(ptrPwd,pszVal->Length + sizeof(wchar_t));
		SecureZeroMemory(pszVal->Buffer,pszVal->Length);
#else
		memset(ptrPwd,0,pszVal->Length + sizeof(wchar_t));
		memset(pszVal->Buffer,0,pszVal->Length);
#endif
		LsaFreeMemory(pszVal);
		LsaClose(hPolicy);

		if (!bRes)
			LOG_ERROR_RETURN(("LogonUserW failed: %s",OOBase::system_error_text(dwErr)),dwErr);

		// Control handle lifetime
		OOBase::Win32::SmartHandle tok(hToken);

		// Restrict the Token
		dwErr = OOSvrBase::Win32::RestrictToken(hToken);
		if (dwErr != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("RestrictToken failed: %s",OOBase::system_error_text(dwErr)),dwErr);

		// This might be needed for retricted tokens...
		dwErr = OOSvrBase::Win32::SetTokenDefaultDACL(hToken);
		if (dwErr != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("SetTokenDefaultDACL failed: %s",OOBase::system_error_text(dwErr)),dwErr);

		tok.detach();
		return ERROR_SUCCESS;
	}

	static DWORD CreateWindowStationSD(TOKEN_USER* pProcessUser, PSID pSIDLogon, OOSvrBase::Win32::sec_descript_t& sd)
	{
		const int NUM_ACES = 3;
		EXPLICIT_ACCESSW ea[NUM_ACES] = { {0}, {0}, {0} };

		// Set minimum access for the calling process SID
		ea[0].grfAccessPermissions = WINSTA_CREATEDESKTOP;
		ea[0].grfAccessMode = GRANT_ACCESS;
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

		ea[1].grfAccessMode = GRANT_ACCESS;
		ea[1].grfInheritance = NO_INHERITANCE;
		ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[1].Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea[1].Trustee.ptstrName = (LPWSTR)pSIDLogon;

		// Set generic all access for Specific logon for everything below...
		ea[2].grfAccessPermissions = STANDARD_RIGHTS_REQUIRED | GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL;
		ea[2].grfAccessMode = GRANT_ACCESS;
		ea[2].grfInheritance = CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
		ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[2].Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea[2].Trustee.ptstrName = (LPWSTR)pSIDLogon;

		return sd.SetEntriesInAcl(NUM_ACES,ea,NULL);
	}

	static DWORD CreateDesktopSD(TOKEN_USER* pProcessUser, PSID pSIDLogon, OOSvrBase::Win32::sec_descript_t& sd)
	{
		const int NUM_ACES = 2;
		EXPLICIT_ACCESSW ea[NUM_ACES] = { {0}, {0} };

		// Set minimum access for the calling process SID
		ea[0].grfAccessPermissions = DESKTOP_CREATEWINDOW;
		ea[0].grfAccessMode = GRANT_ACCESS;
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

		ea[1].grfAccessMode = GRANT_ACCESS;
		ea[1].grfInheritance = NO_INHERITANCE;
		ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[1].Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea[1].Trustee.ptstrName = (LPWSTR)pSIDLogon;

		return sd.SetEntriesInAcl(NUM_ACES,ea,NULL);
	}

	static bool OpenCorrectWindowStation(HANDLE hToken, OOBase::LocalString& strWindowStation, HWINSTA& hWinsta, HDESK& hDesktop)
	{
		// Service window stations are created with the name "Service-0xZ1-Z2$",
		// where Z1 is the high part of the logon SID and Z2 is the low part of the logon SID
		// see http://msdn2.microsoft.com/en-us/library/ms687105.aspx for details

		// Get the logon SID of the Token
		OOBase::SmartPtr<void,OOBase::LocalDestructor> ptrSIDLogon;
		DWORD dwRes = OOSvrBase::Win32::GetLogonSID(hToken,ptrSIDLogon);
		if (dwRes != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("OOSvrBase::Win32::GetLogonSID failed: %s",OOBase::system_error_text(dwRes)),false);

		char* pszSid = 0;
		if (!ConvertSidToStringSidA(ptrSIDLogon,&pszSid))
			LOG_ERROR_RETURN(("ConvertSidToStringSidA failed: %s",OOBase::system_error_text()),false);

		int err = strWindowStation.assign(pszSid);
		LocalFree(pszSid);

		if (err != 0)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

		// Logon SIDs are of the form S-1-5-5-X-Y, and we want X and Y
		if (strncmp(strWindowStation.c_str(),"S-1-5-5-",8) != 0)
			LOG_ERROR_RETURN(("OpenCorrectWindowStation failed: %s",OOBase::system_error_text(ERROR_INVALID_SID)),false);

		// Crack out the last two parts - there is probably an easier way... but this works
		const char* p = strWindowStation.c_str() + 8;
		char* pEnd = 0;
		DWORD dwParts[2];
		dwParts[0] = strtoul(p,&pEnd,10);
		if (*pEnd != '-')
			LOG_ERROR_RETURN(("OpenCorrectWindowStation failed: %s",OOBase::system_error_text(ERROR_INVALID_SID)),false);

		dwParts[1] = strtoul(pEnd+1,&pEnd,10);
		if (*pEnd != '\0')
			LOG_ERROR_RETURN(("OpenCorrectWindowStation failed: %s",OOBase::system_error_text(ERROR_INVALID_SID)),false);

		err = strWindowStation.printf("Service-0x%lx-%lx$",dwParts[0],dwParts[1]);
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to format string: %s",OOBase::system_error_text(err)),false);

		// Get the current processes user SID
		OOBase::Win32::SmartHandle hProcessToken;
		if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
			LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::system_error_text()),false);

		OOBase::SmartPtr<TOKEN_USER,OOBase::HeapDestructor> ptrProcessUser = static_cast<TOKEN_USER*>(OOSvrBase::Win32::GetTokenInfo(hProcessToken,TokenUser));
		if (!ptrProcessUser)
			LOG_ERROR_RETURN(("OOSvrBase::Win32::GetTokenInfo failed: %s",OOBase::system_error_text()),false);

		// Open or create the new window station and desktop
		// Now confirm the Window Station exists...
		hWinsta = OpenWindowStationA((LPSTR)strWindowStation.c_str(),FALSE,WINSTA_CREATEDESKTOP);
		if (!hWinsta)
		{
			// See if just doesn't exist yet...
			dwRes = GetLastError();
			if (dwRes != ERROR_FILE_NOT_FOUND)
				LOG_ERROR_RETURN(("OpenWindowStationA failed: %s",OOBase::system_error_text(dwRes)),false);

			OOSvrBase::Win32::sec_descript_t sd;
			dwRes = CreateWindowStationSD(ptrProcessUser,ptrSIDLogon,sd);
			if (dwRes != ERROR_SUCCESS)
				LOG_ERROR_RETURN(("CreateWindowStationSD failed: %s",OOBase::system_error_text(dwRes)),false);

			SECURITY_ATTRIBUTES sa;
			sa.nLength = sizeof(sa);
			sa.bInheritHandle = FALSE;
			sa.lpSecurityDescriptor = sd.descriptor();

			hWinsta = CreateWindowStationA(strWindowStation.c_str(),0,WINSTA_CREATEDESKTOP,&sa);
			if (!hWinsta)
				LOG_ERROR_RETURN(("CreateWindowStationA failed: %s",OOBase::system_error_text()),false);
		}

		// Stash our old
		HWINSTA hOldWinsta = GetProcessWindowStation();
		if (!hOldWinsta)
		{
			dwRes = GetLastError();
			CloseWindowStation(hWinsta);
			LOG_ERROR_RETURN(("GetProcessWindowStation failed: %s",OOBase::system_error_text(dwRes)),false);
		}

		// Swap our Window Station
		if (!SetProcessWindowStation(hWinsta))
		{
			dwRes = GetLastError();
			CloseWindowStation(hWinsta);
			LOG_ERROR_RETURN(("SetProcessWindowStation failed: %s",OOBase::system_error_text(dwRes)),false);
		}

		// Try for the desktop
		wchar_t szDesktop[] = L"default";
		hDesktop = OpenDesktopW(szDesktop,0,FALSE,DESKTOP_CREATEWINDOW);
		if (!hDesktop)
		{
			// See if just doesn't exist yet...
			dwRes = GetLastError();
			if (dwRes != ERROR_FILE_NOT_FOUND)
			{
				SetProcessWindowStation(hOldWinsta);
				CloseWindowStation(hWinsta);
				LOG_ERROR_RETURN(("OpenDesktopW failed: %s",OOBase::system_error_text(dwRes)),false);
			}

			OOSvrBase::Win32::sec_descript_t sd;
			dwRes = CreateDesktopSD(ptrProcessUser,ptrSIDLogon,sd);
			if (dwRes != ERROR_SUCCESS)
			{
				SetProcessWindowStation(hOldWinsta);
				CloseWindowStation(hWinsta);
				LOG_ERROR_RETURN(("CreateDesktopSD failed: %s",OOBase::system_error_text(dwRes)),false);
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
				LOG_ERROR_RETURN(("CreateDesktopW failed: %s",OOBase::system_error_text(dwRes)),false);
			}
		}

		// Revert our Window Station
		SetProcessWindowStation(hOldWinsta);

		err = strWindowStation.append("\\default");
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to append string: %s",OOBase::system_error_text(err)),false);

		return true;
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

DWORD SpawnedProcessWin32::SpawnFromToken(const OOBase::LocalString& strAppPath, HANDLE hToken, OOBase::Win32::SmartHandle& hPipe, bool bSandbox)
{
	OOBase::LocalString strModule;
	if (strAppPath.empty())
	{
		// Get our module name
		char szPath[MAX_PATH];
		if (!GetModuleFileNameA(NULL,szPath,MAX_PATH))
		{
			DWORD dwErr = GetLastError();
			LOG_ERROR_RETURN(("GetModuleFileNameA failed: %s",OOBase::system_error_text(dwErr)),dwErr);
		}

		// Strip off our name, and add OOSvrUser.exe
		PathRemoveFileSpecA(szPath);

		if (!PathAppendA(szPath,"OOSvrUser.exe"))
		{
			DWORD dwErr = GetLastError();
			LOG_ERROR_RETURN(("PathAppendA failed: %s",OOBase::system_error_text(dwErr)),dwErr);
		}

		int err = strModule.assign(szPath);
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),err);
	}
	else
	{
		int err = strModule.assign(strAppPath.c_str());
		if (err == 0)
		{
			if (strModule.length() < 4)
				err = strModule.append(".exe");
			else
			{
				const char* ext = strModule.c_str()+strModule.length()-4;
				bool ok = (ext[0]=='.' && (ext[1]=='e' || ext[1]=='E') && (ext[2]=='x' || ext[2]=='X') && (ext[3]=='e' || ext[3]=='E'));
				if (!ok)
					err = strModule.append(".exe");
			}
		}
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),err);

		OOSvrBase::Logger::log(OOSvrBase::Logger::Information,"Using oosvruser: %s",strModule.c_str());

		if (strModule.length() >= MAX_PATH)
		{
			// Prefix with '\\?\'
			err = strModule.concat("\\\\?\\",strModule.c_str());
			if (err != 0)
				LOG_ERROR_RETURN(("Failed to append string: %s",OOBase::system_error_text(err)),err);
		}
	}

	// Create the named pipe
	OOBase::LocalString strPipe;
	hPipe = CreatePipe(hToken,strPipe);
	if (!hPipe.is_valid())
	{
		DWORD dwErr = GetLastError();
		LOG_ERROR_RETURN(("Failed to create named pipe: %s",OOBase::system_error_text(dwErr)),dwErr);
	}

	int err = 0;
	OOBase::LocalString strCmdLine;
	if (!strModule.empty() && strModule[0] != '"' && strModule.find(' ') != strModule.npos)
	{
		err = strCmdLine.concat("\"",strModule.c_str());
		if (err == 0)
			err = strCmdLine.append("\"");
	}
	else
		err = strCmdLine.assign(strModule.c_str());

	if (err == 0)
	{
		err = strCmdLine.append(" --fork-slave=");
		if (err == 0)
			err = strCmdLine.append(strPipe.c_str());
	}

	if (err != 0)
		LOG_ERROR_RETURN(("Failed to build command line: %s",OOBase::system_error_text(err)),err);

	OOBase::LocalString strWindowStation;
	err = strWindowStation.assign("WinSta0\\default");
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),err);

	// Forward declare these because of goto's
	DWORD dwRes = ERROR_SUCCESS;
	DWORD dwWait;
	STARTUPINFOA startup_info = {0};
	HWINSTA hWinsta = 0;
	HDESK hDesktop = 0;
	DWORD dwFlags = CREATE_UNICODE_ENVIRONMENT | CREATE_DEFAULT_ERROR_MODE;
	HANDLE hDebugEvent = NULL;
	HANDLE hPriToken = 0;
	OOBase::LocalString strTitle;

	// Load up the users profile
	HANDLE hProfile = NULL;
	if (!bSandbox)
	{
		dwRes = OOSvrBase::Win32::LoadUserProfileFromToken(hToken,hProfile);
		if (dwRes == ERROR_PRIVILEGE_NOT_HELD)
			dwRes = ERROR_SUCCESS;
		else if (dwRes != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("OOSvrBase::Win32::LoadUserProfileFromToken failed: %s",OOBase::system_error_text(dwRes)),dwRes);
	}

	// Load the users environment vars
	LPVOID lpEnv = NULL;
	if (!CreateEnvironmentBlock(&lpEnv,hToken,FALSE))
	{
		dwRes = GetLastError();
		LOG_ERROR(("CreateEnvironmentBlock: %s",OOBase::system_error_text(dwRes)));
		goto Cleanup;
	}

	// Get the primary token from the impersonation token
	if (!DuplicateTokenEx(hToken,TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_DEFAULT,NULL,SecurityImpersonation,TokenPrimary,&hPriToken))
	{
		dwRes = GetLastError();
		LOG_ERROR(("DuplicateTokenEx: %s",OOBase::system_error_text(dwRes)));
		goto Cleanup;
	}

	// Init our startup info
	startup_info.cb = sizeof(STARTUPINFOW);
	startup_info.dwFlags = STARTF_USESHOWWINDOW;
	startup_info.wShowWindow = SW_MINIMIZE;

	if (getenv_OMEGA_DEBUG())
	{
#if defined(OMEGA_DEBUG)
		if (IsDebuggerPresent())
			hDebugEvent = CreateEventW(NULL,FALSE,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX");
#endif

		dwFlags |= CREATE_NEW_CONSOLE;

		// Get the names associated with the user SID
		OOBase::SmartPtr<wchar_t,OOBase::LocalDestructor> strUserName;
		OOBase::SmartPtr<wchar_t,OOBase::LocalDestructor> strDomainName;

		if (OOSvrBase::Win32::GetNameFromToken(hPriToken,strUserName,strDomainName) == ERROR_SUCCESS)
		{
			if (bSandbox)
				err = strTitle.printf("%s - %ls\\%ls [Sandbox]",strModule.c_str(),static_cast<const wchar_t*>(strDomainName),static_cast<const wchar_t*>(strUserName));
			else
				err = strTitle.printf("%s - %ls\\%ls",strModule.c_str(),static_cast<const wchar_t*>(strDomainName),static_cast<const wchar_t*>(strUserName));

			if (err == 0)
				startup_info.lpTitle = const_cast<LPSTR>(strTitle.c_str());
		}
	}
	else
	{
		dwFlags |= DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP;

		if (bSandbox)
			OpenCorrectWindowStation(hPriToken,strWindowStation,hWinsta,hDesktop);

		startup_info.lpDesktop = const_cast<LPSTR>(strWindowStation.c_str());
		startup_info.wShowWindow = SW_HIDE;
	}

	// Actually create the process!
	PROCESS_INFORMATION process_info;
	if (!CreateProcessAsUserA(hPriToken,strModule.c_str(),const_cast<LPSTR>(strCmdLine.c_str()),NULL,NULL,FALSE,dwFlags,lpEnv,NULL,&startup_info,&process_info))
	{
		dwRes = GetLastError();
		LOG_ERROR(("CreateProcessAsUserA: %s",OOBase::system_error_text(dwRes)));

		if (hDebugEvent)
			CloseHandle(hDebugEvent);

		goto Cleanup;
	}

	// Attach a debugger if we are debugging
#if defined(OMEGA_DEBUG)
	if (hDebugEvent)
	{
		AttachDebugger(process_info.dwProcessId);

		SetEvent(hDebugEvent);
		CloseHandle(hDebugEvent);
	}
#endif

	// See if process has immediately terminated...
	dwWait = WaitForSingleObject(process_info.hProcess,500);
	if (dwWait != WAIT_TIMEOUT)
	{
		// Process aborted very quickly... this can happen if we have security issues
		if (!GetExitCodeProcess(process_info.hProcess,&dwRes))
			dwRes = ERROR_INTERNAL_ERROR;

		if (dwRes != STILL_ACTIVE)
		{
			LOG_ERROR(("Process exited immediately, exit code: %#x, %s",dwRes,OOBase::system_error_text(dwRes)));

			CloseHandle(process_info.hProcess);
			CloseHandle(process_info.hThread);

			goto Cleanup;
		}
	}
	m_hProcess = process_info.hProcess;

	// Stash handles to close on end...
	m_hProfile = hProfile;
	hProfile = NULL;

	// Duplicate the impersonated token...
	DuplicateToken(hToken,SecurityImpersonation,&m_hToken);

	// And close any others
	CloseHandle(process_info.hThread);

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

bool SpawnedProcessWin32::Spawn(const OOBase::LocalString& strAppPath, HANDLE hToken, OOBase::Win32::SmartHandle& hPipe, bool bSandbox, bool& bAgain)
{
	m_bSandbox = bSandbox;

	DWORD dwRes = SpawnFromToken(strAppPath,hToken,hPipe,bSandbox);
	if (dwRes == ERROR_PRIVILEGE_NOT_HELD)
		bAgain = true;

	return (dwRes == ERROR_SUCCESS);
}

bool SpawnedProcessWin32::IsRunning() const
{
	if (!m_hProcess.is_valid())
		return false;

	return (WaitForSingleObject(m_hProcess,0) == WAIT_TIMEOUT);
}

bool SpawnedProcessWin32::CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed) const
{
	bAllowed = false;

	OOBase::SmartPtr<void,OOBase::LocalDestructor> pSD;
	for (DWORD cbNeeded = 512;;)
	{
		pSD = OOBase::LocalAllocate(cbNeeded);
		if (!pSD)
			LOG_ERROR_RETURN(("Out of memory"),false);

		if (GetFileSecurityA(pszFName,DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,(PSECURITY_DESCRIPTOR)pSD,cbNeeded,&cbNeeded))
			break;

		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			LOG_ERROR_RETURN(("GetFileSecurityA failed: %s",OOBase::system_error_text()),false);
	}

	// Map the generic access rights
	DWORD dwAccessDesired = 0;
	if (bRead)
		dwAccessDesired |= FILE_GENERIC_READ;

	if (bWrite)
		dwAccessDesired |= FILE_GENERIC_WRITE;

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
	BOOL bRes = ::AccessCheck((PSECURITY_DESCRIPTOR)pSD,m_hToken,dwAccessDesired,&generic,&privilege_set,&dwPrivSetSize,&dwAccessGranted,&bAllowedVal);
	DWORD err = GetLastError();

	if (!bRes && err != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("AccessCheck failed: %s",OOBase::system_error_text(err)),false);

	bAllowed = (bAllowedVal == TRUE);
	return true;
}

bool SpawnedProcessWin32::IsSameLogin(HANDLE hToken) const
{
	// Check the SIDs and priviledges are the same...
	OOBase::SmartPtr<TOKEN_GROUPS_AND_PRIVILEGES,OOBase::HeapDestructor> pStats1 = static_cast<TOKEN_GROUPS_AND_PRIVILEGES*>(OOSvrBase::Win32::GetTokenInfo(hToken,TokenGroupsAndPrivileges));
	OOBase::SmartPtr<TOKEN_GROUPS_AND_PRIVILEGES,OOBase::HeapDestructor> pStats2 = static_cast<TOKEN_GROUPS_AND_PRIVILEGES*>(OOSvrBase::Win32::GetTokenInfo(m_hToken,TokenGroupsAndPrivileges));

	if (!pStats1 || !pStats2)
		return false;

	return (pStats1->SidCount==pStats2->SidCount &&
			pStats1->RestrictedSidCount==pStats2->RestrictedSidCount &&
			pStats1->PrivilegeCount==pStats2->PrivilegeCount &&
			OOSvrBase::Win32::MatchSids(pStats1->SidCount,pStats1->Sids,pStats2->Sids) &&
			OOSvrBase::Win32::MatchSids(pStats1->RestrictedSidCount,pStats1->RestrictedSids,pStats2->RestrictedSids) &&
			OOSvrBase::Win32::MatchPrivileges(pStats1->PrivilegeCount,pStats1->Privileges,pStats2->Privileges));
}

bool SpawnedProcessWin32::IsSameUser(HANDLE hToken) const
{
	// The sandbox is a 'unique' user
	if (m_bSandbox)
		return false;

	OOBase::SmartPtr<TOKEN_USER,OOBase::HeapDestructor> ptrUserInfo1 = static_cast<TOKEN_USER*>(OOSvrBase::Win32::GetTokenInfo(hToken,TokenUser));
	if (!ptrUserInfo1)
		return false;

	OOBase::SmartPtr<TOKEN_USER,OOBase::HeapDestructor> ptrUserInfo2 = static_cast<TOKEN_USER*>(OOSvrBase::Win32::GetTokenInfo(m_hToken,TokenUser));
	if (!ptrUserInfo2)
		return false;

	return (EqualSid(ptrUserInfo1->User.Sid,ptrUserInfo2->User.Sid) == TRUE);
}

bool SpawnedProcessWin32::GetRegistryHive(OOBase::String& strSysDir, OOBase::String& strUsersDir, OOBase::LocalString& strHive)
{
	assert(!m_bSandbox);

	if (strSysDir.empty())
	{
		char szBuf[MAX_PATH] = {0};
		HRESULT hr = SHGetFolderPathA(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
		if FAILED(hr)
			LOG_ERROR_RETURN(("SHGetFolderPathA failed: %s",OOBase::system_error_text()),false);

		if (!PathAppendA(szBuf,"Omega Online"))
			LOG_ERROR_RETURN(("PathAppendA failed: %s",OOBase::system_error_text()),false);

		if (!PathFileExistsA(szBuf))
			LOG_ERROR_RETURN(("%s does not exist.",szBuf),false);

		int err = strSysDir.assign(szBuf);
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);
	}

	if (strUsersDir.empty())
	{
		char szBuf[MAX_PATH] = {0};
		HRESULT hr = SHGetFolderPathA(0,CSIDL_LOCAL_APPDATA,m_hToken,SHGFP_TYPE_DEFAULT,szBuf);
		if FAILED(hr)
			LOG_ERROR_RETURN(("SHGetFolderPathA failed: %s",OOBase::system_error_text()),false);

		if (!PathAppendA(szBuf,"Omega Online"))
			LOG_ERROR_RETURN(("PathAppendA failed: %s",OOBase::system_error_text()),false);

		if (!PathFileExistsA(szBuf) && !CreateDirectoryA(szBuf,NULL))
			LOG_ERROR_RETURN(("CreateDirectoryA %s failed: %s",szBuf,OOBase::system_error_text()),false);

		int err = strHive.concat(szBuf,"\\user.regdb");
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to append strings: %s",OOBase::system_error_text(err)),false);
	}
	else
	{
		if (strUsersDir[strUsersDir.length()-1] != '\\' && strUsersDir[strUsersDir.length()-1] != '/')
		{
			int err = strUsersDir.append("\\");
			if (err != 0)
				LOG_ERROR_RETURN(("Failed to assign strings: %s",OOBase::system_error_text(err)),false);
		}

		// Get the names associated with the user SID
		OOBase::SmartPtr<wchar_t,OOBase::LocalDestructor> strUserName;
		OOBase::SmartPtr<wchar_t,OOBase::LocalDestructor> strDomainName;

		DWORD dwErr = OOSvrBase::Win32::GetNameFromToken(m_hToken,strUserName,strDomainName);
		if (dwErr != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("GetNameFromToken failed: %s",OOBase::system_error_text(dwErr)),false);

		int err = 0;
		if (!strDomainName)
			err = strHive.printf("%s%ls.regdb",strUsersDir.c_str(),static_cast<const wchar_t*>(strUserName));
		else
			err = strHive.printf("%s%ls.%ls.regdb",strUsersDir.c_str(),static_cast<const wchar_t*>(strUserName),static_cast<const wchar_t*>(strDomainName));

		if (err != 0)
			LOG_ERROR_RETURN(("Failed to append strings: %s",OOBase::system_error_text(err)),false);
	}

	// Now confirm the file exists, and if it doesn't, copy default_user.regdb
	if (!PathFileExistsA(strHive.c_str()))
	{
		if (strSysDir[strSysDir.length()-1] != '\\' && strSysDir[strSysDir.length()-1] != '/')
		{
			int err = strSysDir.append("\\");
			if (err != 0)
				LOG_ERROR_RETURN(("Failed to assign strings: %s",OOBase::system_error_text(err)),false);
		}

		int err = strSysDir.append("default_user.regdb");
		if (err != 0)
			LOG_ERROR_RETURN(("Failed to append strings: %s",OOBase::system_error_text(err)),false);

		if (!CopyFileA(strSysDir.c_str(),strHive.c_str(),TRUE))
			LOG_ERROR_RETURN(("Failed to copy %s to %s: %s",strSysDir.c_str(),strHive.c_str(),OOBase::system_error_text()),false);

		::SetFileAttributesA(strHive.c_str(),FILE_ATTRIBUTE_NORMAL);

		// Secure the file if (strUsersDir.empty())
		void* ISSUE_11;
	}

	return true;
}

OOBase::SmartPtr<Root::SpawnedProcess> Root::Manager::platform_spawn(OOSvrBase::AsyncLocalSocket::uid_t uid, bool bSandbox, OOBase::String& strPipe, Omega::uint32_t& channel_id, OOBase::SmartPtr<OOServer::MessageConnection>& ptrMC, bool& bAgain)
{
	// Alloc a new SpawnedProcess
	SpawnedProcessWin32* pSpawn32 = new (std::nothrow) SpawnedProcessWin32();
	if (!pSpawn32)
		LOG_ERROR_RETURN(("Out of memory"),(SpawnedProcess*)0);

	OOBase::SmartPtr<Root::SpawnedProcess> pSpawn = pSpawn32;

	// Spawn the process
	OOBase::LocalString strAppName;
	getenv_i("OMEGA_USER_BINARY",strAppName);

	OOBase::Win32::SmartHandle hPipe;
	if (!pSpawn32->Spawn(strAppName,uid,hPipe,bSandbox,bAgain))
		return 0;

	// Wait for the connect attempt
	if (!WaitForConnect(hPipe))
		return 0;

	// Connect up
	int err = 0;
	OOSvrBase::AsyncLocalSocketPtr ptrSocket = Proactor::instance().attach_local_socket((SOCKET)(HANDLE)hPipe,&err);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to attach socket: %s",OOBase::system_error_text(err)),(SpawnedProcess*)0);

	hPipe.detach();

	// Bootstrap the user process...
	channel_id = bootstrap_user(ptrSocket,ptrMC,strPipe);
	if (!channel_id)
		return 0;

	return pSpawn;
}

bool Root::Manager::get_our_uid(OOSvrBase::AsyncLocalSocket::uid_t& uid, OOBase::LocalString& strUName)
{
	if (uid != INVALID_HANDLE_VALUE)
		CloseHandle(uid);

	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,&uid))
		LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::system_error_text()),false);

	// Get the names associated with the user SID
	OOBase::SmartPtr<wchar_t,OOBase::LocalDestructor> ptrUserName;
	OOBase::SmartPtr<wchar_t,OOBase::LocalDestructor> ptrDomainName;

	DWORD dwRes = OOSvrBase::Win32::GetNameFromToken(uid,ptrUserName,ptrDomainName);
	if (dwRes != ERROR_SUCCESS)
	{
		CloseHandle(uid);
		LOG_ERROR_RETURN(("OOSvrBase::Win32::GetNameFromToken failed: %s",OOBase::system_error_text(dwRes)),false);
	}

	if (!ptrDomainName)
		dwRes = strUName.printf("%ls",static_cast<const wchar_t*>(ptrUserName));
	else
		dwRes = strUName.printf("%ls\\%ls",static_cast<const wchar_t*>(ptrDomainName),static_cast<const wchar_t*>(ptrUserName));
	if (dwRes != 0)
	{
		CloseHandle(uid);
		LOG_ERROR_RETURN(("Failed to format string: %s",OOBase::system_error_text(dwRes)),false);
	}

	// Restrict the Token
	dwRes = OOSvrBase::Win32::RestrictToken(uid);
	if (dwRes != ERROR_SUCCESS)
	{
		CloseHandle(uid);
		LOG_ERROR_RETURN(("OOSvrBase::Win32::RestrictToken failed: %s",OOBase::system_error_text(dwRes)),false);
	}

	return true;
}

bool Root::Manager::get_sandbox_uid(const OOBase::String& strUName, OOSvrBase::AsyncLocalSocket::uid_t& uid, bool& bAgain)
{
	DWORD dwErr = LogonSandboxUser(strUName,uid);
	if (dwErr == ERROR_ACCESS_DENIED)
		bAgain = true;

	return (dwErr == ERROR_SUCCESS);
}

#endif // _WIN32
