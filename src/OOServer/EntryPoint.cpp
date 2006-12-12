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

#include <ace/OS.h>

// Forward declare UserMain
int UserMain(u_short uPort);

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
#ifdef ACE_WIN32
	
	// Check to see if we have been spawned
	if (argc==3 && ACE_OS::strcmp(argv[1],"--spawned")==0)
		return UserMain(static_cast<u_short>(ACE_OS::atoi(argv[2])));

	int ret = NTService::open(argc,argv);
	
#else

	// Daemonize ourselves
	ACE_TCHAR szCwd[MAXPATHLEN];
	ACE_OS::getcwd(szCwd,MAXPATHLEN);
	int ret = ACE::daemonize(szCwd,0,argv[0]);

	// Do something with the cmdline here if we want...

#endif

	if (ret == 0)
	{
		ret = RootManager::run_event_loop();
	}
	
	if (ret < 0)
	{
		ACE_DEBUG((LM_DEBUG,ACE_TEXT("OOServer terminated with error: exitcode = %d, error = %m.\n"),ret));

#ifdef _DEBUG
		// Give us a chance to read the error message
		ACE_OS::printf(ACE_TEXT("OOServer will now wait for 10 seconds so you can read this message...\n"));
		ACE_OS::sleep(10);
#endif
	}

	return ret;
}
