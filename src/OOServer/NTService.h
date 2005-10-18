#pragma once

#include <ace/NT_Service.h>

#if defined (ACE_NT_SERVICE_DEFINE)

#include <ace/Event.h>

#define NTSERVICE_NAME		ACE_TEXT("OOService")
#define NTSERVICE_DESC		ACE_TEXT("Omega Online Network Service")
#define NTSERVICE_LONGDESC	ACE_TEXT("Manages the peer connections for the Omega Online system")

#include "../OOSvc/Shutdown.h"

class NTService : 
	public ACE_NT_Service,
	public OOSvc_Shutdown_Observer
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

	int handle_exception(ACE_HANDLE);
	void handle_control(DWORD control_code);
	int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask mask);

private:
	static ACE_THR_FUNC_RETURN start_service(void*);
	static BOOL WINAPI ctrlc_handler(DWORD dwCtrlType);

	void handle_shutdown();

	bool scm_started_;
	ACE_Event m_finished;
	bool m_our_close;
};

#endif // defined (ACE_NT_SERVICE_DEFINE)
