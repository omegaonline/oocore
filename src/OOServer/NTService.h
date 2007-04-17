/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary and do not use precompiled headers
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_NT_SERVICE_H_INCLUDED_
#define OOSERVER_NT_SERVICE_H_INCLUDED_

#ifdef ACE_WIN32

#define NTSERVICE_NAME		ACE_TEXT("OOServer")
#define NTSERVICE_DESC		ACE_TEXT("Omega Online Network Gateway")
#define NTSERVICE_LONGDESC	ACE_TEXT("Manages the peer connections for the Omega Online network")

namespace Root
{

class NTService : public ACE_NT_Service
{
public:
	NTService(void);
	virtual ~NTService(void);

	static int open(int argc, ACE_TCHAR* argv[]);

	int svc(void);
	
protected:
	int description(const ACE_TCHAR *desc);
	int insert(	const ACE_TCHAR *cmd_line = 0,
				DWORD start_type = SERVICE_DEMAND_START,
				DWORD error_control = SERVICE_ERROR_IGNORE,
				const ACE_TCHAR *group_name = 0,
				LPDWORD tag_id = 0,
				const ACE_TCHAR *dependencies = 0,
				const ACE_TCHAR *account_name = 0,
				const ACE_TCHAR *password = 0);

private:
	typedef ACE_Singleton<NTService, ACE_Recursive_Thread_Mutex> NTSERVICE;

	static ACE_THR_FUNC_RETURN start_service(void*);

	void stop_requested (DWORD control_code);
	void pause_requested (DWORD control_code);
	void continue_requested (DWORD control_code);
	
	ACE_Event m_finished;
};

}

#endif // ACE_WIN32

#endif // OOSERVER_NT_SERVICE_H_INCLUDED_
