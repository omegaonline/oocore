///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
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
#include "./Version.h"

// Forward declare UserMain
int UserMain(const ACE_CString& strPipe);

static int Install(int argc, ACE_TCHAR* argv[])
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
	ACE_OS::fprintf(stdout,ACE_TEXT("OOServer - The OmegaOnline network deamon.\n\n"));
	ACE_OS::fprintf(stdout,ACE_TEXT("Please consult the documentation at http://www.omegaonline.org.uk for further information.\n\n"));
	return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	int skip_args = 1;

#if defined(ACE_WIN32)
	// Check to see if we have been spawned
	if (argc>2 && ACE_OS::strcmp(argv[1],ACE_TEXT("--spawned"))==0)
		return UserMain(ACE_TEXT_ALWAYS_CHAR(argv[2]));

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
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error parsing cmdline")),-1);
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
			ACE_OS::printf("Missing argument for %s.\n\n",cmd_opts.last_option());
			return Help();

		default:
			ACE_OS::printf("Invalid argument '%s'.\n\n",cmd_opts.last_option());
			return Help();
		}
	}

#if defined(ACE_WIN32)

	if ((argc<2 || ACE_OS::strcmp(argv[1],ACE_TEXT("--service")) != 0) && !IsDebuggerPresent())
		ACE_ERROR_RETURN((LM_ERROR,L"OOServer must be started as a Win32 service.\n"),-1);

#if defined(OMEGA_DEBUG)
	if (!IsDebuggerPresent() || ACE_LOG_MSG->open(ACE_TEXT("OOServer"),ACE_Log_Msg::STDERR,ACE_TEXT("OOServer")) != 0)
#endif
	if (ACE_LOG_MSG->open(ACE_TEXT("OOServer"),ACE_Log_Msg::SYSLOG,ACE_TEXT("OOServer")) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error opening logger")),-1);

	if (!Root::NTService::open())
		return -1;

#else

#if !defined(OMEGA_DEBUG)
    // Daemonize ourselves
	ACE_TCHAR szCwd[PATH_MAX];
	ACE_OS::getcwd(szCwd,PATH_MAX);

	if (ACE::daemonize(szCwd,1,ACE_TEXT("ooserverd")) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error daemonizing")),-1);

    if (ACE_LOG_MSG->open(ACE_TEXT("ooserverd"),ACE_Log_Msg::SYSLOG) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error opening logger")),-1);
#else
	if (ACE_LOG_MSG->open(ACE_TEXT("ooserverd"),ACE_Log_Msg::STDERR) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error opening logger")),-1);
#endif

	// TODO - Install signal handlers...
#endif

	int ret = Root::Manager::run(argc - cmd_opts.opt_ind(),&argv[cmd_opts.opt_ind()]);

#if defined(ACE_WIN32)
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
// Leave this function last, because we include <OOCore/config-guess.h> which might be dangerous!
#include <OOCore/config-guess.h>
#include <OOCore/Version.h>
static int Version()
{
	ACE_OS::printf("Version: %hs\nPlatform: %hs\nCompiler: %hs\nACE %hs\n",OOSERVER_VERSION,OMEGA_PLATFORM_STRING,OMEGA_COMPILER_STRING,ACE_VERSION);
	return 0;
}
