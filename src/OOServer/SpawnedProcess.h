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
		SpawnedProcess();
		~SpawnedProcess();

		bool Spawn(uid_t id, u_short uPort, ACE_WString& strSource);
		bool IsRunning();

		bool CheckAccess(const wchar_t* pszFName, ACE_UINT32 mode, bool& bAllowed);

		static bool ResolveTokenToUid(uid_t token, ACE_CString& uid, ACE_WString& strSource);
		static bool GetSandboxUid(ACE_CString& uid);
		static bool InstallSandbox(int argc, wchar_t* argv[]);
		static bool UninstallSandbox();

#if defined(ACE_WIN32)
	protected:
		HANDLE	m_hToken;

		static DWORD LogonSandboxUser(HANDLE* phToken);
		static bool LogFailure(DWORD err);

	private:
		HANDLE	m_hProfile;
		HANDLE	m_hProcess;

		DWORD LoadUserProfileFromToken(HANDLE hToken, HANDLE& hProfile, ACE_WString& strSource);
		DWORD SpawnFromToken(HANDLE hToken, u_short uPort, bool bLoadProfile, ACE_WString& strSource);
		bool unsafe_sandbox();
#else // !ACE_WIN32
	protected:
		uid_t	m_uid;

	private:
		pid_t	m_pid;
		bool CleanEnvironment();

	public:
		static ACE_CString get_home_dir();
#endif // ACE_WIN32

	};

	/*class SpawnedThread : public SpawnedProcess
	{
	public:
		SpawnedThread();
		~SpawnedThread();

		virtual bool Spawn(uid_t id, u_short uPort, ACE_WString& strSource);
		virtual bool IsRunning();

	private:
		ACE_thread_t m_thread_id;

		struct Params
		{
			uid_t   id;
			u_short uPort;
		};
		static ACE_THR_FUNC_RETURN worker_fn(void* pParam);
		static bool LogonUser(uid_t id);
	};*/
}

#endif // OOSERVER_SPAWNED_PROCESS_H_INCLUDED_
