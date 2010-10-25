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

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#define APPNAME "OOSvrUser"
#else
#define APPNAME "oosvruser"
#endif

static int Help()
{
	std::cout << APPNAME " - The Omega Online user host process." << std::endl;
	std::cout << std::endl;
	std::cout << "Please consult the documentation at http://www.omegaonline.org.uk for further information." << std::endl;
	std::cout << std::endl;

	return EXIT_SUCCESS;
}

static int Version()
{
	std::cout << APPNAME " version information:" << std::endl;
	std::cout << "Version: " << OOCORE_VERSION;
#if defined(OMEGA_DEBUG)
	std::cout << " (Debug build)";
#endif
	std::cout << std::endl;
	std::cout << "Compiler: " << OMEGA_COMPILER_STRING << std::endl;
	std::cout << std::endl;

	return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
	// Start the logger - use OOServer again...
	OOSvrBase::Logger::open("OOServer");

	// Set up the command line args
	OOSvrBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
	cmd_args.add_option("version",'v');
	cmd_args.add_option("fork-slave",0,true);

#if !defined(_WIN32)
	cmd_args.add_option("launch-session",0,true);
#endif

	// Parse command line
	std::map<std::string,std::string> args;
	if (!cmd_args.parse(argc,argv,args))
		return EXIT_FAILURE;

	if (args.find("help") != args.end())
		return Help();

	if (args.find("version") != args.end())
		return Version();

#if defined(_WIN32) && defined(OMEGA_DEBUG)
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

#if defined(HAVE_UNISTD_H)
	// Ignore SIGPIPE
	if (signal(SIGPIPE,SIG_IGN) == SIG_ERR)
		LOG_ERROR(("signal(SIGPIPE) failed: %s",OOBase::strerror(errno).c_str()));

	// Ignore SIGCHLD
	if (signal(SIGCHLD,SIG_IGN) == SIG_ERR)
		LOG_ERROR(("signal(SIGCHLD) failed: %s",OOBase::strerror(errno).c_str()));
#endif

	std::string strPipe;
	bool bForkSlave = false;

	// Try to work out how we are being asked to start
	std::map<std::string,std::string>::const_iterator i=args.find("fork-slave");
	if (i != args.end())
	{
		// Fork start from ooserverd
		strPipe = i->second;
		bForkSlave = true;
	}
	else
	{
		if ((i=args.find("launch-session")) != args.end())
		{
			// Start from oo-launch
			strPipe = i->second;
		}
		else
		{
			// Ooops...
			std::cerr << APPNAME " - Invalid or missing arguments" << std::endl;
			return EXIT_FAILURE;
		}
	}

	User::Manager manager;

	// Start the handler
	if (!manager.start_request_threads())
		return EXIT_FAILURE;

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

	if (!bRun)
	{
#if defined(OMEGA_DEBUG)
		// Give us a chance to read the errors!
		OOBase::Thread::sleep(OOBase::timeval_t(5,0));
#endif
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

namespace OOBase
{
	// This is the critical failure hook
	void CriticalFailure(const char* msg)
	{
		std::cerr << msg << std::endl << std::endl;
		abort();
	}
}
