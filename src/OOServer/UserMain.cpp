///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
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

#include "./OOServer_User.h"
#include "./UserManager.h"
#include "../Common/Version.h"

#include <sqlite3.h>

static int Version()
{
	ACE_OS::printf("OOSvrUser version information:\n");
#if defined(OMEGA_DEBUG)
	ACE_OS::printf("Version: %s (Debug build)\nPlatform: %s\nCompiler: %s\nACE %s\n",OOCORE_VERSION,OMEGA_PLATFORM_STRING,OMEGA_COMPILER_STRING,ACE_VERSION);
#else
	ACE_OS::printf("Version: %s\nPlatform: %s\nCompiler: %s\nACE %s\n",OOCORE_VERSION,OMEGA_PLATFORM_STRING,OMEGA_COMPILER_STRING,ACE_VERSION);
#endif

	ACE_OS::printf("\nOOCore version information:\n");
	ACE_OS::printf("%ls\n\n",Omega::System::GetVersion().c_str());

	ACE_OS::printf("SQLite version: %s\n",SQLITE_VERSION);

	ACE_OS::printf("\n");
	return 0;
}

static int Help()
{
	ACE_OS::printf("OOSvrUser - The Omega Online user gateway.\n\n");
	ACE_OS::printf("This program can not be run directly by the user.\n\n");
	ACE_OS::printf("Please consult the documentation at http://www.omegaonline.org.uk for further information.\n\n");
	return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	// Check command line options
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":vh"),1);
	if (cmd_opts.long_option(ACE_TEXT("version"),ACE_TEXT('v'))!=0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error parsing cmdline")),-1);
	
	int option;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case ACE_TEXT('v'):
			return Version();

		case ACE_TEXT(':'):
			ACE_OS::fprintf(stdout,ACE_TEXT("Missing argument for %s.\n\n"),cmd_opts.last_option());
			return Help();

		default:
			ACE_OS::fprintf(stdout,ACE_TEXT("Invalid argument '%s'.\n\n"),cmd_opts.last_option());
			return Help();
		}
	}

	// Check to see if we have been spawned
	if (argc!=2)
	{
		ACE_OS::fprintf(stdout,ACE_TEXT("Invalid or missing arguments.\n\n"));
		return Help();
	}

#if defined(OMEGA_WIN32)
	u_long options = ACE_Log_Msg::SYSLOG;

#if defined(OMEGA_DEBUG)
	options |= ACE_Log_Msg::STDERR;

	// If this event exists, then we are being debugged
	HANDLE hDebugEvent = OpenEventW(EVENT_ALL_ACCESS,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX");
	if (hDebugEvent)
	{
		options = ACE_Log_Msg::STDERR;

		// Wait for a bit, letting the caller attach a debugger
		WaitForSingleObject(hDebugEvent,60000);
		CloseHandle(hDebugEvent);
	}
#endif

	if (ACE_LOG_MSG->open(ACE_TEXT("OOServer"),options,ACE_TEXT("OOServer")) != 0)
        ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("Error opening logger")),-1);

#else // OMEGA_WIN32

    if (ACE_LOG_MSG->open(ACE_TEXT("ooserverd"),ACE_Log_Msg::STDERR | ACE_Log_Msg::SYSLOG) != 0)
        ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("Error opening logger")),-1);

#endif

#if defined(OMEGA_DEBUG)
	Version();
#endif

	return User::Manager::run(ACE_TEXT_ALWAYS_CHAR(argv[1]));
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
