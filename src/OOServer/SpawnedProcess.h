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

namespace Root
{
	class SpawnedProcess
	{
	public:
		SpawnedProcess(void);
		~SpawnedProcess(void);

		bool Spawn(uid_t id, u_short uPort, ACE_CString& strSource);
		
		bool IsRunning();
		int Close(ACE_Time_Value* timeout = 0);
		void Kill();
		bool CheckAccess(const char* pszFName, ACE_UINT32 mode, bool& bAllowed);

		static bool ResolveTokenToUid(uid_t token, ACE_CString& uid, ACE_CString& strSource);
		static bool GetSandboxUid(ACE_CString& uid);
		static bool InstallSandbox();
		static bool UninstallSandbox();

	private:

#if defined(ACE_WIN32)
		HANDLE	m_hToken;
		HANDLE	m_hProfile;
		HANDLE	m_hProcess;
		
		DWORD LoadUserProfileFromToken(HANDLE hToken, HANDLE& hProfile, ACE_CString& strSource);
		DWORD SpawnFromToken(HANDLE hToken, u_short uPort, bool bLoadProfile, ACE_CString& strSource);
		static DWORD LogonSandboxUser(HANDLE* phToken);
		static bool LogFailure(DWORD err);
#else // !ACE_WIN32
		
#endif // ACE_WIN32

	};
}

#endif // OOSERVER_SPAWNED_PROCESS_H_INCLUDED_
