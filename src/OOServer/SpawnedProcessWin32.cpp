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
#include "SpawnedProcess.h"

#if defined(_WIN32)

#include <sddl.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <ntsecapi.h>

static std::string getenv_i(const char* val)
{
#if defined(_MSC_VER) && defined(_CRT_INSECURE_DEPRECATE)
	char* buf = 0;
	size_t len = 0;
	std::string ret;
	if (!_dupenv_s(&buf,&len,val))
	{
		if (len)
			ret = std::string(buf,len-1);
		free(buf);
	}
	return ret;
#else
	return getenv(val);
#endif
}

void AttachDebugger(DWORD pid);

namespace
{
	class SpawnedProcessWin32 : public Root::SpawnedProcess
	{
	public:
		SpawnedProcessWin32();
		virtual ~SpawnedProcessWin32();

		bool Spawn(const std::wstring& strAppPath, HANDLE hToken, OOBase::Win32::SmartHandle& hPipe, bool bSandbox, bool& bAgain);
		bool CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed) const;
		bool Compare(OOSvrBase::AsyncLocalSocket::uid_t uid) const;
		bool IsSameUser(OOSvrBase::AsyncLocalSocket::uid_t uid) const;
		bool GetRegistryHive(const std::string& strSysDir, const std::string& strUsersDir, std::string& strHive);

	private:
		bool                       m_bSandbox;
		OOBase::Win32::SmartHandle m_hToken;
		OOBase::Win32::SmartHandle m_hProcess;
		HANDLE                     m_hProfile;

		DWORD SpawnFromToken(std::wstring strAppPath, HANDLE hToken, OOBase::Win32::SmartHandle& hPipe, bool bSandbox);
	};

	static HANDLE CreatePipe(HANDLE hToken, std::string& strPipe)
	{
		// Create a new unique pipe
		std::ostringstream ssPipe;
		ssPipe.imbue(std::locale::classic());
		ssPipe.setf(std::ios_base::hex);
		ssPipe << "OOR";

		// Get the logon SID of the Token
		OOBase::SmartPtr<void,OOBase::FreeDestructor<void> > ptrSIDLogon = 0;
		DWORD dwRes = OOSvrBase::Win32::GetLogonSID(hToken,ptrSIDLogon);
		if (dwRes != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("GetLogonSID failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),INVALID_HANDLE_VALUE);

		char* pszSid;
		if (ConvertSidToStringSidA(ptrSIDLogon,&pszSid))
		{
			ssPipe << pszSid;
			LocalFree(pszSid);
		}

		OOBase::timeval_t now = OOBase::timeval_t::gettimeofday();
		ssPipe << "-" << now.tv_usec();
		strPipe = ssPipe.str();

		// Create security descriptor
		PSID pSID;
		SID_IDENTIFIER_AUTHORITY SIDAuthCreator = {SECURITY_CREATOR_SID_AUTHORITY};
		if (!AllocateAndInitializeSid(&SIDAuthCreator, 1,
									  SECURITY_CREATOR_OWNER_RID,
									  0, 0, 0, 0, 0, 0, 0,
									  &pSID))
		{
			LOG_ERROR_RETURN(("AllocateAndInitializeSid failed: %s",OOBase::Win32::FormatMessage().c_str()),INVALID_HANDLE_VALUE);
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
			LOG_ERROR_RETURN(("SetEntriesInAcl failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),INVALID_HANDLE_VALUE);

		// Create security attribute
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = FALSE;
		sa.lpSecurityDescriptor = sd.descriptor();

		// Create the named pipe instance
		HANDLE hPipe = CreateNamedPipeA(("\\\\.\\pipe\\" + strPipe).c_str(),
										PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
										PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
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

	static DWORD LogonSandboxUser(const std::wstring& strUName, HANDLE& hToken)
	{
		// Open the local account policy...
		std::wstring strPwd;
		LSA_HANDLE hPolicy;
		LSA_OBJECT_ATTRIBUTES oa = {0};
		DWORD dwErr = LsaNtStatusToWinError(LsaOpenPolicy(NULL,&oa,POLICY_GET_PRIVATE_INFORMATION,&hPolicy));
		if (dwErr != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("LsaOpenPolicy failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),dwErr);

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
			LOG_ERROR_RETURN(("LsaRetrievePrivateData failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),dwErr);
		}

		strPwd.assign(pszVal->Buffer,pszVal->Length / sizeof(wchar_t));
		LsaFreeMemory(pszVal);
		LsaClose(hPolicy);

		if (!LogonUserW((LPWSTR)strUName.c_str(),NULL,(LPWSTR)strPwd.c_str(),LOGON32_LOGON_BATCH,LOGON32_PROVIDER_DEFAULT,&hToken))
		{
			dwErr = GetLastError();
			LOG_ERROR_RETURN(("LogonUserW failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),dwErr);
		}

		// Control handle lifetime
		OOBase::Win32::SmartHandle tok(hToken);

		// Restrict the Token
		dwErr = OOSvrBase::Win32::RestrictToken(hToken);
		if (dwErr != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("RestrictToken failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),dwErr);

		// This might be needed for retricted tokens...
		dwErr = OOSvrBase::Win32::SetTokenDefaultDACL(hToken);
		if (dwErr != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("SetTokenDefaultDACL failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),dwErr);

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

	static bool OpenCorrectWindowStation(HANDLE hToken, std::wstring& strWindowStation, HWINSTA& hWinsta, HDESK& hDesktop)
	{
		// Service window stations are created with the name "Service-0xZ1-Z2$",
		// where Z1 is the high part of the logon SID and Z2 is the low part of the logon SID
		// see http://msdn2.microsoft.com/en-us/library/ms687105.aspx for details

		// Get the logon SID of the Token
		OOBase::SmartPtr<void,OOBase::FreeDestructor<void> > ptrSIDLogon = 0;
		DWORD dwRes = OOSvrBase::Win32::GetLogonSID(hToken,ptrSIDLogon);
		if (dwRes != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("OOSvrBase::Win32::GetLogonSID failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),false);

		wchar_t* pszSid = 0;
		if (!ConvertSidToStringSidW(ptrSIDLogon,&pszSid))
			LOG_ERROR_RETURN(("ConvertSidToStringSidW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

		strWindowStation = pszSid;
		LocalFree(pszSid);

		// Logon SIDs are of the form S-1-5-5-X-Y, and we want X and Y
		if (wcsncmp(strWindowStation.c_str(),L"S-1-5-5-",8) != 0)
			LOG_ERROR_RETURN(("OpenCorrectWindowStation failed: %s",OOBase::Win32::FormatMessage(ERROR_INVALID_SID).c_str()),false);

		// Crack out the last two parts - there is probably an easier way... but this works
		strWindowStation.erase(0,8);
		const wchar_t* p = strWindowStation.c_str();
		wchar_t* pEnd = 0;
		DWORD dwParts[2];
		dwParts[0] = wcstoul(p,&pEnd,10);
		if (*pEnd != L'-')
			LOG_ERROR_RETURN(("OpenCorrectWindowStation failed: %s",OOBase::Win32::FormatMessage(ERROR_INVALID_SID).c_str()),false);

		dwParts[1] = wcstoul(pEnd+1,&pEnd,10);
		if (*pEnd != L'\0')
			LOG_ERROR_RETURN(("OpenCorrectWindowStation failed: %s",OOBase::Win32::FormatMessage(ERROR_INVALID_SID).c_str()),false);

		wchar_t szBuf[128] = {0};
		wsprintfW(szBuf,L"Service-0x%lu-%lu$",dwParts[0],dwParts[1]);
		strWindowStation = szBuf;

		// Get the current processes user SID
		OOBase::Win32::SmartHandle hProcessToken;
		if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
			LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

		OOBase::SmartPtr<TOKEN_USER,OOBase::FreeDestructor<TOKEN_USER> > ptrProcessUser = static_cast<TOKEN_USER*>(OOSvrBase::Win32::GetTokenInfo(hProcessToken,TokenUser));
		if (!ptrProcessUser)
			LOG_ERROR_RETURN(("OOSvrBase::Win32::GetTokenInfo failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

		// Open or create the new window station and desktop
		// Now confirm the Window Station exists...
		hWinsta = OpenWindowStationW((LPWSTR)strWindowStation.c_str(),FALSE,WINSTA_CREATEDESKTOP);
		if (!hWinsta)
		{
			// See if just doesn't exist yet...
			dwRes = GetLastError();
			if (dwRes != ERROR_FILE_NOT_FOUND)
				LOG_ERROR_RETURN(("OpenWindowStationW failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),false);

			OOSvrBase::Win32::sec_descript_t sd;
			dwRes = CreateWindowStationSD(ptrProcessUser,ptrSIDLogon,sd);
			if (dwRes != ERROR_SUCCESS)
				LOG_ERROR_RETURN(("CreateWindowStationSD failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),false);

			SECURITY_ATTRIBUTES sa;
			sa.nLength = sizeof(sa);
			sa.bInheritHandle = FALSE;
			sa.lpSecurityDescriptor = sd.descriptor();

			hWinsta = CreateWindowStationW(strWindowStation.c_str(),0,WINSTA_CREATEDESKTOP,&sa);
			if (!hWinsta)
				LOG_ERROR_RETURN(("CreateWindowStationW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);
		}

		// Stash our old
		HWINSTA hOldWinsta = GetProcessWindowStation();
		if (!hOldWinsta)
		{
			dwRes = GetLastError();
			CloseWindowStation(hWinsta);
			LOG_ERROR_RETURN(("GetProcessWindowStation failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),false);
		}

		// Swap our Window Station
		if (!SetProcessWindowStation(hWinsta))
		{
			dwRes = GetLastError();
			CloseWindowStation(hWinsta);
			LOG_ERROR_RETURN(("SetProcessWindowStation failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),false);
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
				LOG_ERROR_RETURN(("OpenDesktopW failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),false);
			}

			OOSvrBase::Win32::sec_descript_t sd;
			dwRes = CreateDesktopSD(ptrProcessUser,ptrSIDLogon,sd);
			if (dwRes != ERROR_SUCCESS)
			{
				SetProcessWindowStation(hOldWinsta);
				CloseWindowStation(hWinsta);
				LOG_ERROR_RETURN(("CreateDesktopSD failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),false);
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
				LOG_ERROR_RETURN(("CreateDesktopW failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),false);
			}
		}

		// Revert our Window Station
		SetProcessWindowStation(hOldWinsta);

		strWindowStation += L"\\default";

		return true;
	}

	static DWORD GetUserCwd(std::wstring& strCurDir)
	{
		// Find the cwd
		HKEY hKey;
		LONG dwRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Omega Online\\Installation",0,KEY_READ,&hKey);
		if (dwRes == ERROR_SUCCESS)
		{
			wchar_t szPath[MAX_PATH] = {0};
			DWORD dwSize = sizeof(szPath);
			dwRes = RegQueryValueExW(hKey,L"CommonComponents",NULL,NULL,(LPBYTE)&szPath,&dwSize);
			if (dwRes == ERROR_SUCCESS)
				strCurDir = szPath;
			else if (dwRes != ERROR_FILE_NOT_FOUND)
				LOG_ERROR(("Failed to query CommonComponents: %s",OOBase::Win32::FormatMessage(dwRes).c_str()));
						
			RegCloseKey(hKey);
		}
		else if (dwRes != ERROR_FILE_NOT_FOUND)
			LOG_ERROR_RETURN(("Failed to access HKEY_LOCAL_MACHINE\\SOFTWARE\\Omega Online\\Installation: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),dwRes);
		
		if (dwRes == ERROR_FILE_NOT_FOUND)
		{
			// Compose the dir ourselves
			wchar_t szPath[MAX_PATH] = {0};
			HRESULT hr = SHGetFolderPathW(0,CSIDL_PROGRAM_FILES_COMMON,0,SHGFP_TYPE_DEFAULT,szPath);
			if FAILED(hr)
			{
				dwRes = GetLastError();
				LOG_ERROR_RETURN(("SHGetFolderPathW failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),dwRes);
			}
			else
				dwRes = ERROR_SUCCESS;

			strCurDir = szPath;
			strCurDir += L"\\OmegaOnline\\";
		}		

		if (dwRes == ERROR_SUCCESS)
		{
			if (GetFileAttributesW(strCurDir.c_str()) == INVALID_FILE_ATTRIBUTES)
				strCurDir.clear();
		}

		return dwRes;
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

DWORD SpawnedProcessWin32::SpawnFromToken(std::wstring strAppPath, HANDLE hToken, OOBase::Win32::SmartHandle& hPipe, bool bSandbox)
{
	std::wstring strCurDir;
	DWORD dwRes = GetUserCwd(strCurDir);
	if (dwRes != ERROR_SUCCESS)
		return dwRes;

	wchar_t szPath[MAX_PATH];	
	if (strAppPath.empty())
	{
		// Get our module name
		if (!GetModuleFileNameW(NULL,szPath,MAX_PATH))
		{
			DWORD dwErr = GetLastError();
			LOG_ERROR_RETURN(("GetModuleFileNameW failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),dwErr);
		}

		// Strip off our name, and add OOSvrUser.exe
		PathRemoveFileSpecW(szPath);

		if (!PathAppendW(szPath,L"OOSvrUser.exe"))
		{
			DWORD dwErr = GetLastError();
			LOG_ERROR_RETURN(("PathAppendW failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),dwErr);
		}

		strAppPath = szPath;
	}
	else
	{
		if (strAppPath.size() > 4 && strAppPath.substr(strAppPath.size()-4) != L".exe")
			strAppPath += L".exe";

		size_t len = (strAppPath.size()+1 > MAX_PATH ? MAX_PATH : strAppPath.size()+1);
		memcpy(szPath,strAppPath.c_str(),len*sizeof(wchar_t));
		szPath[len-1] = L'\0';

		// Strip off our name
		PathRemoveFileSpecW(szPath);

		// Restore path contents
		len = (strAppPath.size()+1 > MAX_PATH ? MAX_PATH : strAppPath.size()+1);
		memcpy(szPath,strAppPath.c_str(),len*sizeof(wchar_t));
		szPath[len-1] = L'\0';

		OOSvrBase::Logger::log(OOSvrBase::Logger::Warning,"Using oosvruser: %ls",strAppPath.c_str());
	}

	PathQuoteSpacesW(szPath);

	// Create the named pipe
	std::string strPipe;
	hPipe = CreatePipe(hToken,strPipe);
	if (!hPipe.is_valid())
	{
		DWORD dwErr = GetLastError();
		LOG_ERROR_RETURN(("Failed to create named pipe: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),dwErr);
	}

	std::wstring strCmdLine = szPath;
	strCmdLine += L" --fork-slave=" + OOBase::from_native(strPipe.c_str());

	OOBase::SmartPtr<wchar_t,OOBase::ArrayDestructor<wchar_t> > ptrCmdLine = 0;
	OOBASE_NEW(ptrCmdLine,wchar_t[strCmdLine.size()+1]);
	if (!ptrCmdLine)
		LOG_ERROR_RETURN(("PathRemoveFileSpecW failed: %s",OOBase::Win32::FormatMessage(ERROR_OUTOFMEMORY).c_str()),ERROR_OUTOFMEMORY);

	memcpy(ptrCmdLine,strCmdLine.c_str(),strCmdLine.size()*sizeof(wchar_t));
	ptrCmdLine[strCmdLine.size()] = L'\0';

	// Forward declare these because of goto's
	DWORD dwWait;
	STARTUPINFOW startup_info = {0};
	std::wstring strWindowStation;
	HWINSTA hWinsta = 0;
	HDESK hDesktop = 0;
	DWORD dwFlags = CREATE_UNICODE_ENVIRONMENT | CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_PROCESS_GROUP;
	HANDLE hDebugEvent = NULL;
	HANDLE hPriToken = 0;
	std::wstring strTitle;
	
	// Load up the users profile
	HANDLE hProfile = NULL;
	dwRes = OOSvrBase::Win32::LoadUserProfileFromToken(hToken,hProfile);
	if (dwRes == ERROR_PRIVILEGE_NOT_HELD)
		dwRes = ERROR_SUCCESS;
	else if (dwRes != ERROR_SUCCESS)
		LOG_ERROR_RETURN(("OOSvrBase::Win32::LoadUserProfileFromToken failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),dwRes);
	
	// Load the users environment vars
	LPVOID lpEnv = NULL;
	if (!CreateEnvironmentBlock(&lpEnv,hToken,FALSE))
	{
		dwRes = GetLastError();
		LOG_ERROR(("CreateEnvironmentBlock: %s",OOBase::Win32::FormatMessage(dwRes).c_str()));
		goto Cleanup;
	}

	// Get the primary token from the impersonation token
	if (!DuplicateTokenEx(hToken,TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_DEFAULT,NULL,SecurityImpersonation,TokenPrimary,&hPriToken))
	{
		dwRes = GetLastError();
		LOG_ERROR(("DuplicateTokenEx: %s",OOBase::Win32::FormatMessage(dwRes).c_str()));
		goto Cleanup;
	}

	// Init our startup info
	startup_info.cb = sizeof(STARTUPINFOW);
	startup_info.dwFlags = STARTF_USESHOWWINDOW;
	startup_info.wShowWindow = SW_MINIMIZE;

	if (getenv_i("OMEGA_DEBUG") == "yes")
	{
#if defined(OMEGA_DEBUG)
		if (IsDebuggerPresent())
			hDebugEvent = CreateEventW(NULL,FALSE,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX");
#endif

		dwFlags |= CREATE_NEW_CONSOLE;

		strTitle = strAppPath;

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

		startup_info.lpTitle = (LPWSTR)strTitle.c_str();
	}
	else
	{
		dwFlags |= DETACHED_PROCESS;

		if (bSandbox && OpenCorrectWindowStation(hPriToken,strWindowStation,hWinsta,hDesktop))
			startup_info.lpDesktop = const_cast<LPWSTR>(strWindowStation.c_str());
		else
		{
			WCHAR sz[] = L"";
			startup_info.lpDesktop = sz;
			startup_info.wShowWindow = SW_HIDE;
		}
	}

	// Actually create the process!
	PROCESS_INFORMATION process_info;
	if (!CreateProcessAsUserW(hPriToken,strAppPath.c_str(),ptrCmdLine,NULL,NULL,FALSE,dwFlags,lpEnv,strCurDir.empty() ? NULL : strCurDir.c_str(),&startup_info,&process_info))
	{
		dwRes = GetLastError();
		LOG_ERROR(("CreateProcessAsUserW: %s",OOBase::Win32::FormatMessage(dwRes).c_str()));

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
	dwWait = WaitForSingleObject(process_info.hProcess,100);
	if (dwWait != WAIT_TIMEOUT)
	{
		// Process aborted very quickly... this can happen if we have security issues
		dwRes = ERROR_INTERNAL_ERROR;

		LOG_ERROR(("Process exited immediately."));

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

bool SpawnedProcessWin32::Spawn(const std::wstring& strAppPath, HANDLE hToken, OOBase::Win32::SmartHandle& hPipe, bool bSandbox, bool& bAgain)
{
	m_bSandbox = bSandbox;

	DWORD dwRes = SpawnFromToken(strAppPath,hToken,hPipe,bSandbox);
	if (dwRes == ERROR_PRIVILEGE_NOT_HELD)
		bAgain = true;
		
	return (dwRes == ERROR_SUCCESS);
}

bool SpawnedProcessWin32::CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed) const
{
	bAllowed = false;
	std::wstring strFName = OOBase::from_utf8(pszFName);

	OOBase::SmartPtr<void,OOBase::FreeDestructor<void> > pSD = 0;
	DWORD cbNeeded = 0;
	if (!GetFileSecurityW(strFName.c_str(),DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,(PSECURITY_DESCRIPTOR)pSD,0,&cbNeeded) && GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
		LOG_ERROR_RETURN(("GetFileSecurityW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

	pSD = malloc(cbNeeded);
	if (!pSD)
		LOG_ERROR_RETURN(("Out of memory"),false);

	if (!GetFileSecurityW(strFName.c_str(),DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,(PSECURITY_DESCRIPTOR)pSD,cbNeeded,&cbNeeded))
		LOG_ERROR_RETURN(("GetFileSecurityW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

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
		LOG_ERROR_RETURN(("AccessCheck failed: %s",OOBase::Win32::FormatMessage(err).c_str()),false);

	bAllowed = (bAllowedVal == TRUE);
	return true;
}

bool SpawnedProcessWin32::Compare(HANDLE hToken) const
{
	if (m_bSandbox)
		return false;

	// Check the SIDs and priviledges are the same...
	OOBase::SmartPtr<TOKEN_GROUPS_AND_PRIVILEGES,OOBase::FreeDestructor<TOKEN_GROUPS_AND_PRIVILEGES> > pStats1 = static_cast<TOKEN_GROUPS_AND_PRIVILEGES*>(OOSvrBase::Win32::GetTokenInfo(hToken,TokenGroupsAndPrivileges));
	OOBase::SmartPtr<TOKEN_GROUPS_AND_PRIVILEGES,OOBase::FreeDestructor<TOKEN_GROUPS_AND_PRIVILEGES> > pStats2 = static_cast<TOKEN_GROUPS_AND_PRIVILEGES*>(OOSvrBase::Win32::GetTokenInfo(m_hToken,TokenGroupsAndPrivileges));

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

	OOBase::SmartPtr<TOKEN_USER,OOBase::FreeDestructor<TOKEN_USER> > ptrUserInfo1 = static_cast<TOKEN_USER*>(OOSvrBase::Win32::GetTokenInfo(hToken,TokenUser));
	if (!ptrUserInfo1)
		return false;

	OOBase::SmartPtr<TOKEN_USER,OOBase::FreeDestructor<TOKEN_USER> > ptrUserInfo2 = static_cast<TOKEN_USER*>(OOSvrBase::Win32::GetTokenInfo(m_hToken,TokenUser));
	if (!ptrUserInfo2)
		return false;

	return (EqualSid(ptrUserInfo1->User.Sid,ptrUserInfo2->User.Sid) == TRUE);
}

bool SpawnedProcessWin32::GetRegistryHive(const std::string& strSysDir, const std::string& strUsersDir, std::string& strHive)
{
	assert(!m_bSandbox);

	if (strUsersDir.empty())
	{
		wchar_t szBuf[MAX_PATH] = {0};
		HRESULT hr = SHGetFolderPathW(0,CSIDL_LOCAL_APPDATA,m_hToken,SHGFP_TYPE_DEFAULT,szBuf);
		if FAILED(hr)
			LOG_ERROR_RETURN(("SHGetFolderPathW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

		if (!PathAppendW(szBuf,L"Omega Online"))
			LOG_ERROR_RETURN(("PathAppendW failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

		if (!PathFileExistsW(szBuf) && !CreateDirectoryW(szBuf,NULL))
			LOG_ERROR_RETURN(("CreateDirectoryW %s failed: %s",OOBase::to_utf8(szBuf).c_str(),OOBase::Win32::FormatMessage().c_str()),false);

		strHive = OOBase::to_utf8(szBuf).c_str();
		if (!strHive.empty() && *strHive.rbegin() != '\\')
			strHive += '\\';

		strHive += "user";
	}
	else
	{
		// Get the names associated with the user SID
		std::wstring strUserName;
		std::wstring strDomainName;

		DWORD dwErr = OOSvrBase::Win32::GetNameFromToken(m_hToken,strUserName,strDomainName);
		if (dwErr != ERROR_SUCCESS)
			LOG_ERROR_RETURN(("GetNameFromToken failed: %s",OOBase::Win32::FormatMessage(dwErr).c_str()),false);

		strHive = strUsersDir + OOBase::to_utf8(strUserName.c_str());
		if (!strDomainName.empty())
			strHive += "." + OOBase::to_utf8(strDomainName.c_str());
	}

	strHive += ".regdb";

	// Now confirm the file exists, and if it doesn't, copy default_user.regdb
	if (!PathFileExistsW(OOBase::from_utf8(strHive.c_str()).c_str()))
	{
		if (!CopyFileW(OOBase::from_utf8((strSysDir + "default_user.regdb").c_str()).c_str(),OOBase::from_utf8(strHive.c_str()).c_str(),TRUE))
			LOG_ERROR_RETURN(("Failed to copy %s to %s: %s",(strSysDir + "default_user.regdb").c_str(),strHive.c_str(),OOBase::Win32::FormatMessage().c_str()),false);

		::SetFileAttributesW(OOBase::from_utf8(strHive.c_str()).c_str(),FILE_ATTRIBUTE_NORMAL);

		// Secure the file if (strUsersDir.empty())
		void* TODO;
	}

	return true;
}

OOBase::SmartPtr<Root::SpawnedProcess> Root::Manager::platform_spawn(OOSvrBase::AsyncLocalSocket::uid_t uid, bool bSandbox, std::string& strPipe, Omega::uint32_t& channel_id, OOBase::SmartPtr<OOServer::MessageConnection>& ptrMC, bool& bAgain)
{
	// Alloc a new SpawnedProcess
	SpawnedProcessWin32* pSpawn32 = 0;
	OOBASE_NEW(pSpawn32,SpawnedProcessWin32);
	if (!pSpawn32)
		LOG_ERROR_RETURN(("Out of memory"),(SpawnedProcess*)0);

	OOBase::SmartPtr<Root::SpawnedProcess> pSpawn = pSpawn32;

	// Spawn the process
	std::wstring strAppName = OOBase::from_native(getenv_i("OMEGA_USER_BINARY").c_str());
	
	OOBase::Win32::SmartHandle hPipe;
	if (!pSpawn32->Spawn(strAppName,uid,hPipe,bSandbox,bAgain))
		return 0;

	// Wait for the connect attempt
	if (!WaitForConnect(hPipe))
		return 0;

	// Connect up
	int err = 0;
	OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket> ptrSocket = Proactor::instance()->attach_local_socket((SOCKET)(HANDLE)hPipe,&err);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to attach socket: %s",OOBase::system_error_text(err).c_str()),(SpawnedProcess*)0);

	hPipe.detach();

	// Bootstrap the user process...
	channel_id = bootstrap_user(ptrSocket,ptrMC,strPipe);
	if (!channel_id)
		return 0;

	return pSpawn;
}

bool Root::Manager::get_our_uid(OOSvrBase::AsyncLocalSocket::uid_t& uid, std::string& strUName)
{
	if (uid != INVALID_HANDLE_VALUE)
		CloseHandle(uid);

	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,&uid))
		LOG_ERROR_RETURN(("OpenProcessToken failed: %s",OOBase::Win32::FormatMessage().c_str()),false);

	// Get the names associated with the user SID
	std::wstring strUserName;
	std::wstring strDomainName;

	DWORD dwRes = OOSvrBase::Win32::GetNameFromToken(uid,strUserName,strDomainName);
	if (dwRes != ERROR_SUCCESS)
	{
		CloseHandle(uid);
		LOG_ERROR_RETURN(("OOSvrBase::Win32::GetNameFromToken failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),false);
	}

	if (strDomainName.empty())
		strUName = OOBase::to_native(strUserName.c_str());
	else
		strUName = OOBase::to_native(strDomainName.c_str()) + "\\" + OOBase::to_native(strUserName.c_str());

	// Restrict the Token
	dwRes = OOSvrBase::Win32::RestrictToken(uid);
	if (dwRes != ERROR_SUCCESS)
	{
		CloseHandle(uid);
		LOG_ERROR_RETURN(("OOSvrBase::Win32::RestrictToken failed: %s",OOBase::Win32::FormatMessage(dwRes).c_str()),false);
	}

	return true;
}

bool Root::Manager::get_sandbox_uid(const std::string& strUName, OOSvrBase::AsyncLocalSocket::uid_t& uid, bool& bAgain)
{
	DWORD dwErr = LogonSandboxUser(OOBase::from_utf8(strUName.c_str()),uid);
	if (dwErr == ERROR_ACCESS_DENIED)
		bAgain = true;

	return (dwErr == ERROR_SUCCESS);
}

#endif // _WIN32
