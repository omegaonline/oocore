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
#include "./RootManager.h"
#include "./UserSession.h"

#include <ace/OS.h>
#include <ace/Proactor.h>

static ACE_THR_FUNC_RETURN worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ACE_Proactor::instance()->proactor_run_event_loop();
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
	
	// Start the NT Service or unix daemon
	int ret = StartDaemonService(argc,argv);
	if (ret == 1)
	{
		// Return of 1, means, "ok, but close now"
		ret = 0;	
	}
	else if (ret == 0)
	{		
		// Start the Root connection acceptor
		if ((ret=RootManager::ROOT_MANAGER::instance()->init()) == 0)
		{
			// Spawn off the engine threads
			int thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads-1,worker_fn);
			if (thrd_grp_id == -1)
				ret = -1;

			if (ret==0)
			{
				// Treat this thread as a worker as well
				ret = worker_fn(0);

				// Wait for all the threads to finish
				ACE_Thread_Manager::instance()->wait_grp(thrd_grp_id);
			}

			// Close the Root connection acceptor
			RootManager::ROOT_MANAGER::instance()->term();
		}
	}
	
#ifdef _DEBUG
	// Give us a chance to read the error message
	if (ret < 0)
	{
		ACE_DEBUG((LM_DEBUG,ACE_TEXT("\nTerminating OOServer: exitcode = %d, error = %m.\n\n" \
									 "OOServer will now wait for 10 seconds so you can read this message...\n"),ret));
		ACE_OS::sleep(10);
	}
#endif

	return ret;
}
