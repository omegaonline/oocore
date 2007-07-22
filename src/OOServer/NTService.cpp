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

#include "./OOServer_Root.h"

#if defined(ACE_WIN32)

#include "./NTService.h"
#include "./RootManager.h"

ACE_NT_SERVICE_DEFINE(OOServer,Root::NTService,NTSERVICE_DESC);

Root::NTService::NTService() : ACE_NT_Service(NTSERVICE_NAME,NTSERVICE_DESC)
{
}

Root::NTService::~NTService()
{
}

bool Root::NTService::open()
{	
    // Do the ServiceMain in a separate thread
	if (ACE_Thread_Manager::instance()->spawn(NTService::start_service) == -1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error spawning service thread")),false);
	
	return true;
}

bool Root::NTService::install()
{
	// Stop the service if it is running
	NTSERVICE::instance()->stop_svc();

	// Remove the service config first, this allows us to alter the config
	NTSERVICE::instance()->remove();

	if (NTSERVICE::instance()->insert(ACE_TEXT("--service")) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Service install")),false);

	NTSERVICE::instance()->description(NTSERVICE_LONGDESC);

	return true;
}

bool Root::NTService::uninstall()
{
	// Stop the service if it is running
	NTSERVICE::instance()->stop_svc();
	
	// Uninstall the service
	if (NTSERVICE::instance()->remove() != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Service uninstall")),false);

	return true;
}

int Root::NTService::insert(const ACE_TCHAR *cmd_line,
							DWORD start_type,
							DWORD error_control,
							const ACE_TCHAR *group_name,
							LPDWORD tag_id,
							const ACE_TCHAR *dependencies,
							const ACE_TCHAR *account_name,
							const ACE_TCHAR *password)
{
	ACE_TCHAR this_exe[PATH_MAX + 2];

	if (GetModuleFileNameA(0,this_exe+1,PATH_MAX) == 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("GetModuleFilename failed!\n")),-1);
		
	// Make sure that this_exe is quoted
	this_exe[0] = ACE_TEXT('\"');
	ACE_OS::strcat(this_exe, ACE_TEXT("\""));
	
	ACE_TString exe_path(this_exe);
	exe_path += ACE_TEXT(" ");
	exe_path += cmd_line;

	return ACE_NT_Service::insert(start_type,error_control,exe_path.c_str(),group_name,tag_id,dependencies,account_name,password);
}

int Root::NTService::description(const ACE_TCHAR *desc)
{
	SC_HANDLE svc = this->svc_sc_handle ();
	if (svc == 0)
		return -1;

	SERVICE_DESCRIPTION sdesc;
	sdesc.lpDescription = const_cast<LPTSTR>(desc);

	BOOL ok = ::ChangeServiceConfig2(svc,
							(DWORD)SERVICE_CONFIG_DESCRIPTION,	// Change the description
							(LPVOID)&sdesc);

	return ok ? 0 : -1;
}

static BOOL WINAPI control_c(DWORD /*dwCtrlType*/)
{
	Root::Manager::end();

	return TRUE;
}

ACE_THR_FUNC_RETURN Root::NTService::start_service(void*)
{
	// This blocks running svc
	ACE_NT_SERVICE_RUN(OOServer,NTSERVICE::instance(),ret);
	if (!ret)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Service start failed")));

		SetConsoleCtrlHandler(control_c,TRUE);
	}
	
	return 0;
}

int Root::NTService::svc(void)
{
	report_status(SERVICE_RUNNING);

	// Just wait here until we are told to stop
	int ret = m_finished.wait();

	report_status(SERVICE_STOPPED);

	return ret;
}

void Root::NTService::pause_requested(DWORD)
{
	// We don't pause
	report_status(SERVICE_RUNNING);
}

void Root::NTService::continue_requested(DWORD)
{
	// We don't pause
	report_status(SERVICE_RUNNING);
}

void Root::NTService::stop_requested(DWORD)
{
	report_status(SERVICE_STOP_PENDING);

	Manager::end();

	// Tell the service thread to stop
	m_finished.signal();
}

#endif // ACE_WIN32
