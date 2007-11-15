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

		bool Spawn(Manager::user_id_type id, const ACE_WString& strPipe);
		bool CheckAccess(const wchar_t* pszFName, ACE_UINT32 mode, bool& bAllowed);

		static bool InstallSandbox(int argc, wchar_t* argv[]);
		static bool UninstallSandbox();
		static bool SecureFile(const ACE_WString& strFilename);
		
#if defined(ACE_WIN32)

		bool Compare(HANDLE hToken);
		bool IsSameUser(HANDLE hToken);

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

		bool Compare(uid_t uid)
		{
			return (m_uid == uid);
		}
		bool IsSameUser(uid_t uid)
		{
			return Compare(uid);
		}

		static ACE_CString get_home_dir();

	private:
		uid_t	m_uid;
		pid_t	m_pid;

		bool CleanEnvironment();
		
#endif // ACE_WIN32

	};
}

#endif // OOSERVER_SPAWNED_PROCESS_H_INCLUDED_

