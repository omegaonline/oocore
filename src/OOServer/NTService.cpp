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

#include "./NTService.h"

#ifdef OOSERVER_USE_NTSERVICE

#include "./RootManager.h"

#include <ace/Get_Opt.h>
#include <ace/ARGV.h>
#include <ace/OS.h>
#include <ace/Proactor.h>
#include <ace/SString.h>

// For the Windows path functions
#include <shlwapi.h>

ACE_NT_SERVICE_DEFINE(OOServer,NTService,NTSERVICE_DESC);

NTService::NTService(void) : 
	ACE_NT_Service(NTSERVICE_NAME,NTSERVICE_DESC)
{
}

NTService::~NTService(void)
{
}

int NTService::open(int argc, ACE_TCHAR* argv[])
{
	// Used for cmdline processing
	ACE_ARGV svc_argv;
	int bInstall = 0;
	bool bOptionsAlready = false;
	bool bDebug = false;

	// Check command line options
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":ds:"));
	int option;
	while ((option = cmd_opts()) != EOF && !bInstall)
	{
		switch (option)
		{
		case ACE_TEXT('s'):
			if (ACE_OS::strcasecmp(ACE_TEXT("install"),cmd_opts.opt_arg())==0)
			{
				if (bOptionsAlready)
					ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("-s Install must come before all other command line options.\n")),-1);
				
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

		if (NTSERVICE::instance()->insert(svc_argv.buf()) == -1)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Install")),-1);

		NTSERVICE::instance()->description(NTSERVICE_LONGDESC);

		ACE_OS::printf(ACE_TEXT("%s installed successfully.\n"),NTSERVICE_DESC);
		return 1;
	}
	
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
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) GetModuleFilename failed.\n")),-1);
		
	// Make sure that this_exe is quoted
	this_exe[0] = ACE_TEXT('\"');
	ACE_OS::strcat(this_exe, ACE_TEXT("\""));
	
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
	// This blocks running svc
	ACE_NT_SERVICE_RUN(OOServer,NTSERVICE::instance(),ret);
	
	if (ret == 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),NTSERVICE_NAME),(ACE_THR_FUNC_RETURN)-1);
		
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("%T (%t): Service stopped.\n")));
	return 0;
}

int NTService::svc(void)
{
	report_status(SERVICE_RUNNING);

	// Just wait here until we are told to stop
	m_finished.wait();

	report_status(SERVICE_STOPPED);

	return 0;
}

void NTService::pause_requested(DWORD)
{
	// We don't pause
	report_status(SERVICE_RUNNING);
}

void NTService::continue_requested(DWORD)
{
	// We don't pause
	report_status(SERVICE_RUNNING);
}

void NTService::stop_requested(DWORD)
{
	report_status(SERVICE_STOP_PENDING);

	ACE_DEBUG((LM_INFO,ACE_TEXT ("Service control stop requested.\n")));

	RootManager::ROOT_MANAGER::instance()->close();

	// Tell the service thread to stop
	m_finished.signal();
}

int StartDaemonService(int argc, ACE_TCHAR* argv[])
{
	return NTService::open(argc,argv);
}

#endif // OOSERVER_USE_NTSERVICE
