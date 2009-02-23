///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

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
#include "../Common/Version.h"

static int Install(int argc, ACE_TCHAR* argv[])
{
#if defined(OMEGA_WIN32)
	if (!Root::NTService::install())
		return EXIT_FAILURE;
#endif

	if (!Root::Manager::install(argc,argv))
		return EXIT_FAILURE;

	ACE_OS::printf("Installed successfully.\n");
	return EXIT_SUCCESS;
}

static int Uninstall()
{
#if defined(OMEGA_WIN32)
	if (!Root::NTService::uninstall())
		return EXIT_FAILURE;
#endif

	if (!Root::Manager::uninstall())
		return EXIT_FAILURE;

	ACE_OS::printf("Uninstalled successfully.\n");
	return EXIT_SUCCESS;
}

static int Version();

static int Help()
{
	ACE_OS::printf("OOServer - The Omega Online network deamon.\n\n");
	ACE_OS::printf("Please consult the documentation at http://www.omegaonline.org.uk for further information.\n\n");
	return EXIT_SUCCESS;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	int skip_args = 1;

#if defined(OMEGA_WIN32)
	if (argc>=2 && ACE_OS::strcmp(argv[1],ACE_TEXT("--service"))==0)
		skip_args = 2;
#endif

	// Check command line options
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":iuvh"),skip_args);
	if (cmd_opts.long_option(ACE_TEXT("install"),ACE_TEXT('i'))!=0 ||
		cmd_opts.long_option(ACE_TEXT("uninstall"),ACE_TEXT('u'))!=0 ||
		cmd_opts.long_option(ACE_TEXT("version"),ACE_TEXT('v'))!=0 ||
		cmd_opts.long_option(ACE_TEXT("help"),ACE_TEXT('h'))!=0)
	{
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error parsing cmdline")),EXIT_FAILURE);
	}

	int option;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case ACE_TEXT('i'):
			return Install(argc - cmd_opts.opt_ind(),&argv[cmd_opts.opt_ind()]);

		case ACE_TEXT('u'):
			return Uninstall();

		case ACE_TEXT('v'):
			return Version();

		case ACE_TEXT('h'):
			return Help();

		case ACE_TEXT(':'):
			ACE_OS::fprintf(stdout,ACE_TEXT("Missing argument for %s.\n\n"),cmd_opts.last_option());
			return Help();

		default:
			ACE_OS::fprintf(stdout,ACE_TEXT("Invalid argument '%s'.\n\n"),cmd_opts.last_option());
			return Help();
		}
	}

#if defined(OMEGA_WIN32)
	const ACE_TCHAR* pszAppName = ACE_TEXT("OOServer");

	if (argc<2 || ACE_OS::strcmp(argv[1],ACE_TEXT("--service")) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("OOServer must be started as a Win32 service.\n")),EXIT_FAILURE);

#else
	const ACE_TCHAR* pszAppName = ACE_TEXT("ooserverd");
#endif

	// Start the logger
	int flags = ACE_Log_Msg::STDERR;
#if !defined(OMEGA_DEBUG)
	flags |= ACE_Log_Msg::SYSLOG;
#endif
	if (ACE_LOG_MSG->open(pszAppName,flags,pszAppName) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error opening logger")),EXIT_FAILURE);

	// Start any daemons or services
#if defined(OMEGA_WIN32)
	if (!Root::NTService::open())
		return EXIT_FAILURE;
#else // OMEGA_WIN32
#if !defined(OMEGA_DEBUG)
    // Daemonize ourselves
	ACE_TCHAR szCwd[PATH_MAX];
	ACE_OS::getcwd(szCwd,PATH_MAX);

	if (ACE::daemonize(szCwd,1,pszAppName) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error daemonizing")),EXIT_FAILURE);

#endif // OMEGA_DEBUG

	// Install signal handlers...
	void* TODO;

#endif // OMEGA_WIN32

#if defined(OMEGA_DEBUG)
	Version();
#endif

	// Run the RootManager
	int ret = Root::Manager::run(argc - cmd_opts.opt_ind(),&argv[cmd_opts.opt_ind()]);

#if defined(OMEGA_WIN32)
	Root::NTService::stop();
#endif

	return ret;
}

#if defined(ACE_WIN32) && defined(ACE_USES_WCHAR) && defined(__MINGW32__)
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
// Leave this function last because we includes a lot of headers, which might be dangerous!
#include <OOCore/OOCore.h>
#include <sqlite3.h>
static int Version()
{
	ACE_OS::printf("OOServer version information:\n");
#if defined(OMEGA_DEBUG)
	ACE_OS::printf("Version: %s (Debug build)\nPlatform: %s\nCompiler: %s\nACE %s\n",OOCORE_VERSION,OMEGA_PLATFORM_STRING,OMEGA_COMPILER_STRING,ACE_VERSION);
#else
	ACE_OS::printf("Version: %s\nPlatform: %s\nCompiler: %s\nACE %s\n",OOCORE_VERSION,OMEGA_PLATFORM_STRING,OMEGA_COMPILER_STRING,ACE_VERSION);
#endif

	ACE_OS::printf("\nOOCore version information:\n");
	ACE_OS::printf("%ls\n\n",Omega::System::GetVersion().c_str());

	ACE_OS::printf("SQLite version: %s\n",SQLITE_VERSION);

	ACE_OS::printf("\n");
	return EXIT_SUCCESS;
}
