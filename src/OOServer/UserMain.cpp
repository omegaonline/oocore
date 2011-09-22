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

#if defined(_WIN32) && !defined(__MINGW32__)
#define APPNAME "OOSvrUser"
#else
#define APPNAME "oosvruser"
#endif

namespace
{
	int Help()
	{
		printf(APPNAME " - The Omega Online user host process.\n\n"
			"Please consult the documentation at http://www.omegaonline.org.uk for further information.\n\n");

		return EXIT_SUCCESS;
	}

	int Version()
	{
		printf(APPNAME " version %s",OOCORE_VERSION);

	#if defined(OMEGA_DEBUG)
		printf(" (Debug build)");
	#endif
		printf("\n\tCompiler: %s\n\n",OMEGA_COMPILER_STRING);

		return EXIT_SUCCESS;
	}

	bool CriticalFailure(const char* msg)
	{
		OOSvrBase::Logger::log(OOSvrBase::Logger::Error,msg);
		return true;
	}
}

int main(int argc, char* argv[])
{
	// Start the logger - use OOServer again...
	OOSvrBase::Logger::open("OOServer",__FILE__);

	// Set critical failure handler
	OOBase::SetCriticalFailure(&CriticalFailure);

	// Set up the command line args
	OOBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
	cmd_args.add_option("version",'v');
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
			OOSvrBase::Logger::log(OOSvrBase::Logger::Error,"Missing value for option %s",strErr.c_str());
		else if (args.find("unknown",strErr))
			OOSvrBase::Logger::log(OOSvrBase::Logger::Error,"Unknown option %s",strErr.c_str());
		else
			OOSvrBase::Logger::log(OOSvrBase::Logger::Error,"Failed to parse comand line: %s",OOBase::system_error_text(err));
			
		return EXIT_FAILURE;
	}
	
	if (args.find("help") != args.npos)
		return Help();

	if (args.find("version") != args.npos)
		return Version();

	OOBase::LocalString debug;
	debug.getenv("OMEGA_DEBUG");
	if (debug == "yes")
	{
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
	#endif
	}

#if !defined(_WIN32)
	// Ignore SIGPIPE
	if (::signal(SIGPIPE,SIG_IGN) == SIG_ERR)
		LOG_ERROR(("signal(SIGPIPE) failed: %s",OOBase::system_error_text()));

	// Ignore SIGCHLD
	if (::signal(SIGCHLD,SIG_IGN) == SIG_ERR)
		LOG_ERROR(("signal(SIGCHLD) failed: %s",OOBase::system_error_text()));
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
			OOSvrBase::Logger::log(OOSvrBase::Logger::Error,APPNAME " - Invalid or missing arguments.");
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
		manager.run();

	// Stop the MessageHandler
	manager.stop_request_threads();

	manager.m_proactor_pool.join();

	if (!bRun)
	{
#if defined(OMEGA_DEBUG)
		// Give us a chance to read the errors!
		OOBase::Thread::sleep(OOBase::timeval_t(15,0));
#endif
		LOG_ERROR_RETURN((APPNAME " exiting prematurely."),EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
