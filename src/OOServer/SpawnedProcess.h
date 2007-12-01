///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
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

		bool Spawn(user_id_type id, const ACE_WString& strPipe);
		bool CheckAccess(const wchar_t* pszFName, ACE_UINT32 mode, bool& bAllowed);

		static bool InstallSandbox(int argc, wchar_t* argv[]);
		static bool UninstallSandbox();
		static bool SecureFile(const ACE_WString& strFilename);

		bool Compare(user_id_type hToken);
		bool IsSameUser(user_id_type hToken);
		
#if defined(ACE_WIN32)
	private:
		HANDLE	m_hToken;
		HANDLE	m_hProfile;
		HANDLE	m_hProcess;

		DWORD LoadUserProfileFromToken(HANDLE hToken, HANDLE& hProfile);
		DWORD SpawnFromToken(HANDLE hToken, const ACE_WString& strPipe, bool bLoadProfile);
		void* GetTokenInfo(HANDLE hToken, TOKEN_INFORMATION_CLASS cls);
		bool MatchSids(ULONG count, PSID_AND_ATTRIBUTES pSids1, PSID_AND_ATTRIBUTES pSids2);
		bool MatchPrivileges(ULONG count, PLUID_AND_ATTRIBUTES Privs1, PLUID_AND_ATTRIBUTES Privs2);

		static bool LogonSandboxUser(HANDLE* phToken);
		static bool LogFailure(DWORD err, const wchar_t* pszFile, unsigned int nLine);
		static bool unsafe_sandbox();

#else // !ACE_WIN32
		static ACE_CString get_home_dir();

	private:
		uid_t	m_uid;
		pid_t	m_pid;

		bool CleanEnvironment();
		
#endif // ACE_WIN32

	};
}

#endif // OOSERVER_SPAWNED_PROCESS_H_INCLUDED_

