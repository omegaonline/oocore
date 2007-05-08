/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "./OOServer_Root.h"
#include "./NTService.h"
#include "./RootManager.h"

#include <OOCore/OOCore.h>

// Forward declare UserMain
int UserMain(u_short uPort);

static int Install()
{
#if defined(ACE_WIN32)
	if (!Root::NTService::install())
		return -1;
#endif
	
	if (!Root::Manager::install())
		return -1;

	ACE_OS::printf("Installed successfully.\n");
	return 0;
}

static int Uninstall()
{
#if defined(ACE_WIN32)
	if (!Root::NTService::uninstall())
		return -1;
#endif
	
	if (!Root::Manager::uninstall())
		return -1;

	ACE_OS::printf("Uninstalled successfully.\n");
	return 0;
}

static int Version()
{
	ACE_OS::printf("Version: lu.%lu.%lu\n",OOSERVER_MAJOR_VERSION,OOSERVER_MINOR_VERSION,OOSERVER_BUILD_VERSION);
	ACE_OS::printf("OOCore: lu.%lu.%lu\n",OMEGA_MAJOR_VERSION,OMEGA_MINOR_VERSION,OMEGA_BUILD_VERSION);
	ACE_OS::printf("Platform: %s\nCompiler: %s\n",(OMEGA_PLATFORM_STRING),(OMEGA_COMPILER_STRING));
	ACE_OS::printf("ACE: %lu.%lu.%lu",ACE_MAJOR_VERSION,ACE_MINOR_VERSION,ACE_BETA_VERSION);
	return 0;
}

static int Help()
{
	ACE_OS::printf("This is the help string\n\n");
	return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	int skip_args = 1;

#if defined(ACE_WIN32)
	if (argc>=2 && ACE_OS::strcmp(argv[1],"--service")==0)
		skip_args = 2;

	// Check to see if we have been spawned
	if (argc==3 && ACE_OS::strcmp(argv[1],"--spawned")==0)
		return UserMain(static_cast<u_short>(ACE_OS::atoi(argv[2])));
#endif
	
	// Check command line options
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":iuvh"),skip_args);
	if (cmd_opts.long_option(ACE_TEXT("install"),ACE_TEXT('i'))!=0 ||
		cmd_opts.long_option(ACE_TEXT("uninstall"),ACE_TEXT('u'))!=0 ||
		cmd_opts.long_option(ACE_TEXT("version"),ACE_TEXT('v'))!=0 ||
		cmd_opts.long_option(ACE_TEXT("help"),ACE_TEXT('h'))!=0)
	{
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error parsing cmdline")),-1);
	}

	int option;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case ACE_TEXT('i'):
			return Install();

		case ACE_TEXT('u'):
			return Uninstall();

		case ACE_TEXT('v'):
			return Version();

		case ACE_TEXT('h'):
			return Help();

		case ACE_TEXT(':'):
			ACE_OS::printf("Missing argument for -%c.\n\n",cmd_opts.opt_opt());
			return Help();

		default:
			ACE_OS::printf("Invalid argument '%c'.\n\n",cmd_opts.opt_opt());
			return Help();
		}
	}

#if defined(ACE_WIN32)
	if (argc<2 || ACE_OS::strcmp(argv[1],"--service")!=0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("OOServer must be run as a Win32 service.\n")),-1);
		
	if (ACE_LOG_MSG->open(ACE_TEXT("OOServer"),ACE_Log_Msg::SYSLOG,ACE_TEXT("OOServer")) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error opening logger")),-1);
	
	if (!Root::NTService::open())
		return -1;

#else
	// Daemonize ourselves
	ACE_TCHAR szCwd[MAXPATHLEN];
	ACE_OS::getcwd(szCwd,MAXPATHLEN);
	if (ACE::daemonize(szCwd,0,argv[0]) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error daemonizing")),-1);

	if (ACE_LOG_MSG->open(argv[0],ACE_Log_Msg::SYSLOG) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error opening logger")),-1);
#endif

	return Root::Manager::run();
}
