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

#ifndef OOSERVER_SPAWNED_PROCESS_H_INCLUDED_
#define OOSERVER_SPAWNED_PROCESS_H_INCLUDED_

#include "./RootManager.h"

namespace Root
{
	class SpawnedProcess
	{
	public:
		SpawnedProcess();
		~SpawnedProcess();

		bool Spawn(user_id_type id, const ACE_CString& strPipe, bool bSandbox);
		bool CheckAccess(const char* pszFName, ACE_UINT32 mode, bool& bAllowed);

		static bool InstallSandbox(int argc, ACE_TCHAR* argv[]);
		static bool UninstallSandbox();
		static bool SecureFile(const ACE_CString& strFilename);

		bool Compare(user_id_type uid);
		bool IsSameUser(user_id_type uid);
		ACE_CString GetRegistryHive();

		static bool LogonSandboxUser(user_id_type& uid);
		static void CloseSandboxLogon(user_id_type uid);

#if defined(OMEGA_WIN32)
	private:
		HANDLE m_hToken;
		HANDLE m_hProfile;
		HANDLE m_hProcess;

		static DWORD LoadUserProfileFromToken(HANDLE hToken, HANDLE& hProfile);
		static void* GetTokenInfo(HANDLE hToken, TOKEN_INFORMATION_CLASS cls);
		static bool MatchSids(ULONG count, PSID_AND_ATTRIBUTES pSids1, PSID_AND_ATTRIBUTES pSids2);
		static bool MatchPrivileges(ULONG count, PLUID_AND_ATTRIBUTES Privs1, PLUID_AND_ATTRIBUTES Privs2);
		static DWORD GetNameFromToken(HANDLE hToken, ACE_WString& strUserName, ACE_WString& strDomainName);
		static DWORD GetLogonSID(HANDLE hToken, PSID& pSIDLogon);
		static DWORD OpenCorrectWindowStation(HANDLE hToken, ACE_WString& strWindowStation, HWINSTA& hWinsta, HDESK& hDesktop);
		static DWORD CreateWindowStationSD(TOKEN_USER* pProcessUser, PSID pSIDLogon, PACL& pACL, void*& pSD);
		static DWORD CreateDesktopSD(TOKEN_USER* pProcessUser, PSID pSIDLogon, PACL& pACL, void*& pSD);
		static DWORD CreateSD(PACL pACL, void*& pSD);
		static DWORD SetTokenDefaultDACL(HANDLE hToken);
		static bool RestrictToken(HANDLE& hToken);
		static void EnableUserAccessToFile(LPWSTR pszPath, TOKEN_USER* pUser);
		static bool LogFailure(DWORD err, const wchar_t* pszFile, unsigned int nLine);

		DWORD SpawnFromToken(HANDLE hToken, const ACE_CString& strPipe, bool bSandbox);

#else // !OMEGA_WIN32
		
	private:
		uid_t	m_uid;
		pid_t	m_pid;

		bool CleanEnvironment();
		bool close_all_fds();
		bool linux_close_all_fds();
		bool posix_close_all_fds(long max_fd);

#endif // OMEGA_WIN32

	private:
        bool   m_bSandbox;
	};
}

#endif // OOSERVER_SPAWNED_PROCESS_H_INCLUDED_

