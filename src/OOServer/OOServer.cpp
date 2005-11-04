// OOServer.cpp : Defines the entry point for the application.
//
// ACE includes
#include <ace/ACE.h>
#include <ace/Service_Config.h>
#include <ace/Service_Repository.h>
#include <ace/Reactor.h>
#include <ace/NT_Service.h>
#include <ace/Thread_Manager.h>

#if !defined (ACE_WIN32)
// Only use TP_Reactor when not on Windows
#include <ace/TP_Reactor.h>
#endif

#include "../OOCore/Engine.h"

#include "./NTService.h"
#include "./OOControlService.h"

static ACE_THR_FUNC_RETURN worker_fn(void * p)
{
	ENGINE::instance()->pump_requests();

	return (errno != ESHUTDOWN ? -1 : 0);
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	// Start the engine
	if (ENGINE::instance()->open(argc,argv) != 0)
		return -1;

#if defined (ACE_NT_SERVICE_DEFINE)
	int ret;
	if ((ret = NTService::open(argc,argv))!=0)
		return ret;
#endif // defined (ACE_NT_SERVICE_DEFINE)

	// Load the service configuration file
	if (ACE_Service_Config::open(argc,argv) != 0)
		return -1;
	
	// Start the local control service
	if (OOCONTROL_SERVICE::instance()->open(true) != 0)
		return -1;

	// Spawn off some extra threads
	ACE_Thread_Manager::instance()->spawn_n(3,worker_fn);

	// Run the reactor loop...  
	// This is needed to make ACE_Service_Config work
	ACE_Reactor::instance()->owner(ACE_Thread::self());
	ACE_Reactor::instance()->run_reactor_event_loop();

	// Stop the engine
	ENGINE::instance()->shutdown();

	// Wait for all the threads to finish
	ACE_Thread_Manager::instance()->wait();

	return 0;
}
