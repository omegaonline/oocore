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

#include "./OOServer_Root.h"
#include "./NTService.h"
#include "./RootManager.h"

// Forward declare UserMain
int UserMain(u_short uPort);

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
#ifdef ACE_WIN32

	if (ACE_LOG_MSG->open(ACE_TEXT("OOServer"),ACE_Log_Msg::SYSLOG,ACE_TEXT("OOServer")) != 0)
		return -1;
	
	// Check to see if we have been spawned
	if (argc==3 && ACE_OS::strcmp(argv[1],"--spawned")==0)
		return UserMain(static_cast<u_short>(ACE_OS::atoi(argv[2])));

	int ret = Root::NTService::open(argc,argv);
	
#else

	if (ACE_LOG_MSG->open(argv[0],ACE_Log_Msg::SYSLOG) != 0)
		return -1;

	// Daemonize ourselves
	ACE_TCHAR szCwd[MAXPATHLEN];
	ACE_OS::getcwd(szCwd,MAXPATHLEN);
	int ret = ACE::daemonize(szCwd,0,argv[0]);

	// Do something with the cmdline here if we want...

#endif

	if (ret != 0)
		return ret;

	return Root::Manager::run();
}
