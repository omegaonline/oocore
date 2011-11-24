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

#include "OOServer_User.h"
#include "UserManager.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <sys/stat.h>
#endif

namespace
{
	int Help()
	{
		OOBase::stdout_write(APPNAME " - The Omega Online user host process.\n\n"
			"Please consult the documentation at http://www.omegaonline.org.uk for further information.\n\n");

		return EXIT_SUCCESS;
	}

	int Version()
	{
		OOBase::stdout_write(APPNAME " version " OOCORE_VERSION);

	#if !defined(NDEBUG)
		OOBase::stdout_write(" (Debug build)");
	#endif
		OOBase::stdout_write("\n\tCompiler: " OMEGA_COMPILER_STRING "\n\n");

		return EXIT_SUCCESS;
	}

	bool CriticalFailure(const char* msg)
	{
		OOBase::Logger::log(OOBase::Logger::Error,msg);

		if (User::is_debug())
		{
			// Give us a chance to read the errors!
			OOBase::Thread::sleep(OOBase::timeval_t(15));
		}
		return true;
	}

	static bool s_is_debug = false;
}

bool User::is_debug()
{
	return s_is_debug;
}

int main(int argc, char* argv[])
{
	// Start the logger - use OOServer again...
	OOBase::Logger::open("OOServer",__FILE__);

	// Set critical failure handler
	OOBase::SetCriticalFailure(&CriticalFailure);

	// Set up the command line args
	OOBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
	cmd_args.add_option("version",'v');
	cmd_args.add_option("debug");
	cmd_args.add_option("fork-slave",0,true);

#if !defined(_WIN32)
	cmd_args.add_option("launch-session",0,true);
#endif

	// Parse command line
	OOBase::CmdArgs::results_t args;
	int err = cmd_args.parse(argc,argv,args);
	if (err	!= 0)
	{
		OOBase::String strErr;
		if (args.find("missing",strErr))
			OOBase::Logger::log(OOBase::Logger::Error,"Missing value for option %s",strErr.c_str());
		else if (args.find("unknown",strErr))
			OOBase::Logger::log(OOBase::Logger::Error,"Unknown option %s",strErr.c_str());
		else
			OOBase::Logger::log(OOBase::Logger::Error,"Failed to parse comand line: %s",OOBase::system_error_text(err));
			
		return EXIT_FAILURE;
	}
	
	s_is_debug = args.exists("debug");

	if (args.exists("help"))
		return Help();

	if (args.exists("version"))
		return Version();

#if defined(_WIN32)
	// If this event exists, then we are being debugged
	// Scope it...
	{
		OOBase::Win32::SmartHandle hDebugEvent(OpenEventW(EVENT_ALL_ACCESS,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX"));
		if (hDebugEvent)
		{
			// Wait for a bit, letting the caller attach a debugger
			WaitForSingleObject(hDebugEvent,5000);
		}
	}

#else

	// Ignore SIGCHLD
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);

	umask(0);

#endif

	OOBase::String strPipe;
	bool bForkSlave = false;

	// Try to work out how we are being asked to start
	if (args.find("fork-slave",strPipe))
	{
		// Fork start from ooserverd
		bForkSlave = true;
	}
	else
	{
		if (!args.find("launch-session",strPipe))
		{
			// Ooops...
			OOBase::Logger::log(OOBase::Logger::Error,APPNAME " - Invalid or missing arguments.");
			return EXIT_FAILURE;
		}
	}

	User::Manager manager;

	if (!manager.start_proactor_threads())
		return EXIT_FAILURE;

	// Start the handler
	if (!manager.start_request_threads(2))
	{
		manager.stop_request_threads();
		return EXIT_FAILURE;
	}

	// Do the correct init
	bool bRun = false;
	if (bForkSlave)
		bRun = manager.fork_slave(strPipe);
	else
		bRun = manager.session_launch(strPipe);

	// Now run...
	if (bRun)
	{
		OOBase::Logger::log(OOBase::Logger::Debug,APPNAME " started successfully");

		manager.run();
	}

	// Stop the manager
	manager.stop();

	if (User::is_debug() && !bRun)
	{
		OOBase::Logger::log(OOBase::Logger::Debug,"\nPausing to let you read the messages...");

		// Give us a chance to read the errors!
		OOBase::Thread::sleep(OOBase::timeval_t(15));
	}

	return (bRun ? EXIT_SUCCESS : EXIT_FAILURE);
}
