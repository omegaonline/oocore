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
#include "./Version.h"

// Forward declare UserMain
int UserMain(const ACE_WString& strPipe);

static int Install(int argc, wchar_t* argv[])
{
#if defined(ACE_WIN32)
	if (!Root::NTService::install())
		return -1;
#endif

	if (!Root::Manager::install(argc,argv))
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

static int Version();

static int Help()
{
	void* TODO;

	ACE_OS::printf("This is the help string... I'm sure something will go here at some point\n\n");
	return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	int skip_args = 1;

#if defined(ACE_WIN32)
	// Check to see if we have been spawned
	if (argc==3 && ACE_OS::strcmp(argv[1],L"--spawned")==0)
		return UserMain(argv[2]);

	if (argc>=2 && ACE_OS::strcmp(argv[1],L"--service")==0)
		skip_args = 2;
#endif

	// Check command line options
	ACE_Get_Opt cmd_opts(argc,argv,L":iuvh",skip_args);
	if (cmd_opts.long_option(L"install",L'i')!=0 ||
		cmd_opts.long_option(L"uninstall",L'u')!=0 ||
		cmd_opts.long_option(L"version",L'v')!=0 ||
		cmd_opts.long_option(L"help",L'h')!=0)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Error parsing cmdline"),-1);
	}

	int option;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case L'i':
			return Install(argc - cmd_opts.opt_ind(),&argv[cmd_opts.opt_ind()]);

		case L'u':
			return Uninstall();

		case L'v':
			return Version();

		case L'h':
			return Help();

		case L':':
			ACE_OS::printf("Missing argument for %ls.\n\n",cmd_opts.last_option());
			return Help();

		default:
			{
				ACE_OS::printf("Invalid argument '%ls'.\n\n",cmd_opts.last_option());
				return Help();
			}
		}
	}

#if defined(ACE_WIN32)
	if (!IsDebuggerPresent() && (argc<2 || ACE_OS::strcmp(argv[1],L"--service")!=0))
		ACE_ERROR_RETURN((LM_ERROR,L"OOServer must be started as a Win32 service.\n"),-1);

	if (ACE_LOG_MSG->open(L"OOServer",ACE_Log_Msg::SYSLOG,L"OOServer") != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Error opening logger"),-1);

	if (!Root::NTService::open())
		return -1;

#else
	// Daemonize ourselves
	wchar_t szCwd[PATH_MAX];
	ACE_OS::getcwd(szCwd,PATH_MAX);
	if (ACE::daemonize(szCwd,1,argv[0]) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Error daemonizing"),-1);

	if (ACE_LOG_MSG->open(argv[0],ACE_Log_Msg::SYSLOG) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Error opening logger"),-1);

	// TODO - Install signal handlers...
#endif

	return Root::Manager::run(argc - cmd_opts.opt_ind(),&argv[cmd_opts.opt_ind()]);
}

#if defined(ACE_WIN32) && defined(__MINGW32__)
#include <shellapi.h>
int main(int argc, char* /*argv*/[])
{
	// MinGW doesn't understand wmain, so...
	wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(),&argc);

	ACE_Main m;
	return ace_os_wmain_i (m, argc, wargv);   /* what the user calls "main" */
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////
// Leave this function last, because we include <OOCore/config-guess.h> which might be dangerous!
#include <OOCore/config-guess.h>
#include <OOCore/Version.h>
static int Version()
{
	ACE_OS::printf("Version: %s\nPlatform: %s\nCompiler: %s\nACE %s\n",OOSERVER_VERSION,OMEGA_PLATFORM_STRING,OMEGA_COMPILER_STRING,ACE_VERSION);
	return 0;
}

