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

#ifndef OOSERVER_NT_SERVICE_H_INCLUDED_
#define OOSERVER_NT_SERVICE_H_INCLUDED_

#if defined(ACE_WIN32)

#define NTSERVICE_NAME		L"OOServer"
#define NTSERVICE_DESC		L"Omega Online Network Gateway"
#define NTSERVICE_LONGDESC	L"Manages the peer connections for the Omega Online network"

namespace Root
{
	class NTService : public ACE_NT_Service
	{
	public:
		NTService();
		virtual ~NTService();

		static bool open();
		static bool install();
		static bool uninstall();
		
	private:
		typedef ACE_Singleton<NTService, ACE_Thread_Mutex> NTSERVICE;

		static ACE_THR_FUNC_RETURN start_service(void*);
		static BOOL WINAPI control_c(DWORD);
		
		int description(const wchar_t *desc);
		int insert(	const wchar_t *cmd_line = 0,
					DWORD start_type = SERVICE_DEMAND_START,
					DWORD error_control = SERVICE_ERROR_IGNORE,
					const wchar_t *group_name = 0,
					LPDWORD tag_id = 0,
					const wchar_t *dependencies = 0,
					const wchar_t *account_name = 0,
					const wchar_t *password = 0);

		int svc();
		void stop_requested(DWORD control_code);
		void pause_requested(DWORD control_code);
		void continue_requested(DWORD control_code);
		
		ACE_Event m_finished;
	};
}

#endif // ACE_WIN32

#endif // OOSERVER_NT_SERVICE_H_INCLUDED_
