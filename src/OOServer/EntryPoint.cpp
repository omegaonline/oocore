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

#define ACE_HAS_VERSIONED_NAMESPACE  1
#define ACE_AS_STATIC_LIBS

#include <ace/Auto_Ptr.h>
#include <ace/Get_Opt.h>

#include "./NTService.h"
#include "./RootManager.h"
#include "./UserSession.h"

static ACE_THR_FUNC_RETURN worker_reactor_fn(void * p)
{
	try
	{
		int ret = ACE_Reactor::instance()->run_reactor_event_loop();
		if (ret != 0)
		{
			if (ACE_OS::last_error() == ESHUTDOWN)
				ret = 0;
		}
		return ret;
	}
	catch (...)
	{
		return (ACE_THR_FUNC_RETURN)-1;
	}
}

#ifdef ACE_WIN32
int RootMain(int argc, ACE_TCHAR* argv[]);
int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	// Check to see if we have been spawned
	if (argc==3 && ACE_OS::strcmp(argv[1],"--spawned")==0)
		return UserMain(static_cast<u_short>(ACE_OS::atoi(argv[2])));

	return RootMain(argc,argv);
}
int RootMain(int argc, ACE_TCHAR* argv[])
#else
int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
#endif
{
	// Determine default threads from processor count
	int threads = ACE_OS::num_processors();
	if (threads < 2)
		threads = 2;
	
	// Parse cmd line...
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":t:"));
	int option;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case ACE_TEXT('t'):
			threads = ACE_OS::atoi(cmd_opts.opt_arg());
			if (threads<1 || threads>8)
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Bad number of threads '%s' range is [2..8].\n"),cmd_opts.opt_arg()),-1);
			break;

		case ACE_TEXT(':'):
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Missing argument for -%c.\n"),cmd_opts.opt_opt()),-1);
			break;

		default:
			break;
		}
	}

	// Start the reactor
	int ret = 0;
	if ((ret = StartReactor()) == 0)
	{
		// Start the NT Service or unix daemon
		ret = StartDaemonService(argc,argv);
		if (ret == 1)
		{
			// Return of 1, means, "ok, but close now"
			ret = 0;	
		}
		else if (ret == 0)
		{		
			// Start the Root connection acceptor
			if ((ret=RootManager::ROOT_MANAGER::instance()->open()) == 0)
			{
				// Spawn off the engine threads
				int reactor_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads,worker_reactor_fn);
				if (reactor_thrd_grp_id == -1)
					ret = -1;

				if (ret==0)
				{
					// Treat this thread as a reactor_worker as well
					ret = worker_reactor_fn(0);

					// Wait for all the threads to finish
					ACE_Thread_Manager::instance()->wait_grp(reactor_thrd_grp_id);
				}

				// Close the Root connection acceptor
				RootManager::ROOT_MANAGER::instance()->close();
			}
		}
	}

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
