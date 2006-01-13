// OOServer.cpp : Defines the entry point for the application.

#include <ace/Service_Config.h>
#include <ace/NT_Service.h>
#include <ace/Thread_Manager.h>
#include <ace/Get_Opt.h>

#include "../OOCore/Engine.h"

#include "./NTService.h"
#include "./Client_Connection.h"

static ACE_THR_FUNC_RETURN worker_fn(void * p)
{
	OOCore::ENGINE::instance()->pump_requests();

	return static_cast<ACE_THR_FUNC_RETURN>(errno != ESHUTDOWN ? -1 : 0);
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	// Parse cmd line first
	// NB - ACE_Service_Config uses "bdf:k:nyp:s:S:"
	//      Engine uses "e:"

#ifdef _DEBUG
	ACE_TCHAR buf[MAXPATHLEN];
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("Starting OOServer in working directory:\n\t'%s'\n\n"),ACE_OS::getcwd(buf,MAXPATHLEN)));
#endif

	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":t:"));
	int option;
	int threads = ACE_OS::num_processors()+1;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case ACE_TEXT('t'):
			threads = ACE_OS::atoi(cmd_opts.opt_arg());
			if (threads<1 || threads>10)
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Bad number of threads '%s' range is [1..10].\n"),cmd_opts.opt_arg()),-1);
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

	// Start the engine
	if ((ret=OOCore::ENGINE::instance()->open(argc,argv)) == 0)
	{
		// Start the local connection acceptor
		if ((ret=Client_Connection::init()) == 0)
		{
			// Load the service configuration file
			if ((ret=ACE_Service_Config::open(argc,argv)) == 0)
			{	
				// Spawn off some extra threads
				ACE_Thread_Manager::instance()->spawn_n(threads,worker_fn);

				// Run the reactor loop...  
				// This is needed to make ACE_Service_Config work
				ACE_Reactor::instance()->owner(ACE_Thread::self());
				ACE_Reactor::run_event_loop();

				// Close the services
				ACE_Service_Config::fini_svcs();
			}
		}

		// Stop the engine
		OOCore::ENGINE::instance()->close();
	}

	// Wait for all the threads to finish
	ACE_Thread_Manager::instance()->wait();

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
