#include "./NTService.h"

#ifdef ACE_NT_SERVICE_DEFINE

#include <ace/OS.h>
#include <ace/SString.h>
#include <ace/Get_opt.h>
#include <ace/ARGV.h>
#include <ace/Service_Config.h>
#include <ace/Reactor.h>
#include <ace/Service_Repository.h>
#include <ace/Service_Types.h>
#include <ace/Singleton.h>
#include <ace/Mutex.h>

// For the Windows path functions
#include <shlwapi.h>

// Define a singleton class as a way to insure that there's only one
// Service instance in the program, and to protect against access from
// multiple threads.  The first reference to it at runtime creates it,
// and the ACE_Object_Manager deletes it at run-down.

typedef ACE_Singleton<NTService, ACE_Thread_Mutex> NTSERVICE;

ACE_NT_SERVICE_DEFINE(OOServer,NTService,NTSERVICE_DESC);

NTService::NTService(void) : 
	ACE_NT_Service(NTSERVICE_NAME,NTSERVICE_DESC),
	scm_started_(false),
	m_our_close(false)
{
}

NTService::~NTService(void)
{
}

int NTService::open(int argc, ACE_TCHAR* argv[])
{
	// Used for cmdline processing
	ACE_ARGV svc_argv(0);
	int bInstall = 0;
	bool bRunAsService = false;
	bool bOptionsAlready = false;
	bool bDebug = false;
	
	// Check command line options, these must match those of ACE_Service_Config::parse_args
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":bdf:k:nyp:s:S:"));
	int option;
	while ((option = cmd_opts()) != EOF && !bInstall)
	{
		switch (option)
		{
		case ACE_TEXT('s'):
			if (bRunAsService)
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Do not mix -b and -s.\n")),-1);
			else if (ACE_OS::strcasecmp(ACE_TEXT("install"),cmd_opts.opt_arg())==0)
			{
				if (bOptionsAlready)
					ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("-s Install must come before all other command line options.\n")),-1);
				
				svc_argv.add(ACE_TEXT("-b"));
				bInstall = cmd_opts.opt_ind();
			}
			else if (ACE_OS::strcasecmp(ACE_TEXT("uninstall"),cmd_opts.opt_arg())==0)
			{
				// Uninstall the service
				if (NTSERVICE::instance()->remove() == -1)
					ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Uninstall")),-1);

				ACE_OS::printf(ACE_TEXT("%s uninstalled successfully.\n"),NTSERVICE_DESC);
				return 1;
			}
			else
				ACE_ERROR_RETURN((LM_WARNING,ACE_TEXT("Invalid argument for -%c %s.\n"),cmd_opts.opt_opt(),cmd_opts.opt_arg()),-1);
			break;

		case ACE_TEXT('b'):
			if (bInstall)
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Do not mix -b and -s.\n")),-1);
			
			bRunAsService = true;
			break;

		case ACE_TEXT('d'):
			bDebug = true;
			break;

		case ACE_TEXT(':'):
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Missing argument for -%c.\n"),cmd_opts.opt_opt()),-1);
			break;

		default:
			bOptionsAlready = true;
			break;
		}
	}

	if (bInstall)
	{
		for (int i=bInstall;i<argc;++i)
		{
			svc_argv.add(argv[i]);
		}

		// Remove the service config first,
		// this allows us to alter the config
		NTSERVICE::instance()->remove();

		if (NTSERVICE::instance()->insert(svc_argv.buf(),SERVICE_AUTO_START) == -1)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Install")),-1);

		NTSERVICE::instance()->description(NTSERVICE_LONGDESC);

		ACE_OS::printf(ACE_TEXT("%s installed successfully.\n"),NTSERVICE_DESC);
		return 1;
	}
	
	if (bRunAsService)
	{
		// Change working directory to current exe directory
		ACE_TCHAR this_exe[MAXPATHLEN];
		if (!ACE_TEXT_GetModuleFileName(0,this_exe,MAXPATHLEN) ||
			!::PathRemoveFileSpec(this_exe) || 
			!::SetCurrentDirectory(this_exe))
		{
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to change working dir\n")),-1);
		}
      
#ifdef _DEBUG
		if (bDebug)
		{
			// Break into debugger from SCM
			::DebugBreak();
		}
#endif // _DEBUG

		// Do the ServiceMain in a seperate thread
		if (ACE_Thread_Manager::instance()->spawn(NTService::start_service)==-1)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed spawn service thread\n")),-1);
	}

	// Enusre we are instantiated
	NTSERVICE::instance();

	// Install ConsoleCtrlHandler for Ctrl+C
	if (!SetConsoleCtrlHandler(ctrlc_handler,TRUE))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),NTSERVICE_NAME),-1);

	return 0;
}

int NTService::insert(const ACE_TCHAR *cmd_line,
						DWORD start_type,
						DWORD error_control,
						const ACE_TCHAR *group_name,
						LPDWORD tag_id,
						const ACE_TCHAR *dependencies,
						const ACE_TCHAR *account_name,
						const ACE_TCHAR *password)
{
	ACE_TCHAR this_exe[MAXPATHLEN + 2];

	if (ACE_TEXT_GetModuleFileName (0, this_exe + 1, MAXPATHLEN) == 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) GetModuleFilename failed.\n")),-1);
		
	// Make sure that this_exe is quoted
	this_exe[0] = ACE_LIB_TEXT ('\"');
	ACE_OS::strcat (this_exe, ACE_LIB_TEXT ("\""));
	
	ACE_TString exe_path(this_exe);
	exe_path += ACE_TEXT(" ");
	exe_path += cmd_line;

	return ACE_NT_Service::insert(start_type,error_control,exe_path.c_str(),group_name,tag_id,dependencies,account_name,password);
}

int NTService::description(const ACE_TCHAR *desc)
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

ACE_THR_FUNC_RETURN NTService::start_service(void*)
{
	// Assume everything is okay
	NTSERVICE::instance()->scm_started_ = true;

	// This blocks running svc
	ACE_NT_SERVICE_RUN(OOServer,NTSERVICE::instance(),ret);
	
	if (ret == 0)
	{
		NTSERVICE::instance()->scm_started_ = false;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),NTSERVICE_NAME),-1);
	}
	
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("%T (%t): Service stopped.\n")));
	return 0;
}

BOOL WINAPI NTService::ctrlc_handler(DWORD dwCtrlType)
{
	ACE_UNUSED_ARG(dwCtrlType);

	ACE_DEBUG ((LM_INFO,ACE_TEXT ("Service control stop requested.\n")));

	return (ACE_Reactor::instance()->notify(NTSERVICE::instance(),ACE_Event_Handler::EXCEPT_MASK)==0 ? TRUE : FALSE);
}

int NTService::svc(void)
{
	report_status(SERVICE_RUNNING, 0);

	// Just wait here until we are told to stop
	return m_finished.wait();
}

void NTService::handle_control(DWORD control_code)
{
	if (control_code == SERVICE_CONTROL_SHUTDOWN || 
		control_code == SERVICE_CONTROL_STOP)
	{
		if (scm_started_)
			report_status(SERVICE_STOP_PENDING);

		ACE_DEBUG ((LM_INFO,ACE_TEXT ("Service control stop requested.\n")));

		ACE_Reactor::instance()->notify(this,ACE_Event_Handler::EXCEPT_MASK);
	}
	else
		ACE_NT_Service::handle_control(control_code);
}

void NTService::handle_shutdown()
{
	if (!m_our_close)
		ACE_Reactor::instance()->notify(this,ACE_Event_Handler::WRITE_MASK);
}

int NTService::handle_exception(ACE_HANDLE)
{
	return -1;
}

int NTService::handle_close(ACE_HANDLE handle, ACE_Reactor_Mask mask)
{
	if (mask & ACE_Event_Handler::EXCEPT_MASK)
	{
		if (!m_our_close)
		{
			m_our_close = true;
//			OOSvc_Shutdown();
			ACE_Reactor::instance()->end_reactor_event_loop();
		}
		else
		{
			// This prevents us getting totally stuck!
			ACE_OS::abort();
		}
	}

	// Tell the service thread to stop
	m_finished.signal();

	// Tell the main reactor to stop
	//ACE_Reactor::instance()->end_reactor_event_loop();
	
	return ACE_NT_Service::handle_close(handle,mask);
}

#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)
template class ACE_Singleton<NTService, ACE_Thread_Mutex>;
#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)
#pragma instantiate ACE_Singleton<NTService, ACE_Thread_Mutex>
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */

#endif // ACE_NT_SERVICE_DEFINE
