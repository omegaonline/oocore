// OOServer.cpp : Defines the entry point for the application.

#include <ace/Service_Config.h>
#include <ace/NT_Service.h>
#include <ace/Thread_Manager.h>
#include <ace/Get_Opt.h>
#include <ace/ARGV.h>

#ifdef ACE_WIN32
// For the Windows path functions
#include <shlwapi.h>
#endif

#include "./NTService.h"
#include "./Client_Connection.h"

static ACE_THR_FUNC_RETURN worker_fn(void * p)
{
	OOCore::PumpRequests();

	// We can't use a C++ cast here because gcc and VC disagree which one is valid!
	return (ACE_THR_FUNC_RETURN)(errno != ESHUTDOWN ? -1 : 0);
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	// Parse cmd line first
	// NB - ACE_Service_Config uses "bdf:k:nyp:s:S:"
	//      Engine uses "e:"
	//		We use "t:"

#ifdef _DEBUG
	ACE_TCHAR buf[MAXPATHLEN];
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("Starting OOServer with working directory:\n\t'%s'\n\n"),ACE_OS::getcwd(buf,MAXPATHLEN)));
#endif

	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":t:"));
	int option;
	bool bSeen_f = false;
	int threads = ACE_OS::num_processors()+1;
	if (threads==0)
		threads = 2;
	
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case ACE_TEXT('t'):
			threads = ACE_OS::atoi(cmd_opts.opt_arg());
			if (threads<1 || threads>10)
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Bad number of threads '%s' range is [1..10].\n"),cmd_opts.opt_arg()),-1);
			break;
			
		case ACE_TEXT('f'):
			bSeen_f = true;
			break;

		case ACE_TEXT(':'):
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Missing argument for -%c.\n"),cmd_opts.opt_opt()),-1);
			break;

		default:
			break;
		}
	}

	// Start the NTService/Signal handlers
	int ret = 0;
#if defined (ACE_NT_SERVICE_DEFINE)
	if ((ret = NTService::open(argc,argv))!=0)
		return ret;
#endif // defined (ACE_NT_SERVICE_DEFINE)

	int thrd_grp_id = -1;

	// Start the engine
	if ((ret=OOCore::OpenEngine(argc,argv)) == 0)
	{
		// Start the local connection acceptor
		if ((ret=Client_Connection::init()) == 0)
		{
			ACE_ARGV svc_argv;
			for (int i=0;i<argc;++i)
			{
				svc_argv.add(argv[i]);
			}
			
			if (!bSeen_f)
			{
#ifdef ACE_WIN32
				ACE_TCHAR szBuf[MAX_PATH] = "\0";
				if (ACE_TEXT_GetModuleFileName(0,szBuf,MAX_PATH)==0)
					ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to detect OOServer config file location\n")),-1);
				
				::PathRemoveFileSpec(szBuf);

				ACE_TCHAR szConf[MAX_PATH] = "-f ";
				ACE_OS::strcat(szConf,szBuf);
				ACE_OS::strcat(szConf,"\\svc.conf");
	
				svc_argv.add(szConf);
#else
				/// @todo Check for environment variable first?
				svc_argv.add(ACE_TEXT("-f /etc/OmegaOnline/svc.conf"));
#endif
			}
			
			// Load the service configuration file
			if ((ret=ACE_Service_Config::open(svc_argv.argc(),svc_argv.argv(),ACE_DEFAULT_LOGGER_KEY,1,1)) == 0)
			{	
				// Spawn off some extra threads
				thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads,worker_fn);

				// Run the reactor loop...  
				// This is needed to make ACE_Service_Config work
				ACE_Reactor::instance()->owner(ACE_Thread::self());
				ACE_Reactor::run_event_loop();

				// Close the services
				ACE_Service_Config::fini_svcs();
			}
		}

		// Stop the engine
		OOCore::CloseEngine();
	}

	// Wait for all the threads to finish
	if (thrd_grp_id != -1)
		ACE_Thread_Manager::instance()->wait_grp(thrd_grp_id);

#ifdef _DEBUG
	// Give us a chance to read the error message
	if (ret != 0)
	{
		ACE_DEBUG((LM_DEBUG,ACE_TEXT("\nTerminating OOServer: exitcode = %d, error = %m.\n\n" \
									 "OOServer will now wait for 10 seconds so you can read this message...\n"),ret));
		ACE_OS::sleep(10);
	}
#endif

	return ret;
}
