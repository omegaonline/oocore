// OOServer.cpp : Defines the entry point for the application.
//
// ACE includes
#include <ace/ACE.h>
#include <ace/Service_Config.h>
#include <ace/Service_Repository.h>
#include <ace/Reactor.h>
#include <ace/NT_Service.h>

#if !defined (ACE_WIN32)
// Only use TP_Reactor when not on Windows
#include <ace/TP_Reactor.h>
#endif

#include "./NTService.h"
#include "./OOControlService.h"

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	// Check for custom reactor usage, before ACE_Service_Config::open()
#if !defined (ACE_WIN32)
	ACE_TP_Reactor* tp_reactor = new ACE_TP_Reactor;
	ACE_Reactor* new_reactor = new ACE_Reactor(tp_reactor,1);
	ACE_Reactor::instance(new_reactor,1);
#endif // !ACE_WIN32

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

	// Run the reactor loop...
	ACE_Reactor::instance()->owner(ACE_Thread::self());
	OOCore_RunReactor();
	
	// Wait for all the threads to finish
	ACE_Thread_Manager::instance()->wait();

	return 0;
}