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

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <sys/stat.h>
#endif

#if defined(_MSC_VER)
// Shutup VS leak
extern "C" int _setenvp() { return 0; }
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
		OOBase::Logger::log(OOBase::Logger::Error,"%s",msg);

		if (User::is_debug())
		{
			// Give us a chance to read the errors!
			OOBase::Thread::sleep(15000);
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

	// Declare a local stack allocator
	OOBase::StackAllocator<1024> allocator;

	// Set up the command line args
	OOBase::CmdArgs cmd_args(allocator);
	cmd_args.add_option("help",'h');
	cmd_args.add_option("version",'v');
	cmd_args.add_option("debug");
	cmd_args.add_option("pipe",0,true);

	// Parse command line
	OOBase::CmdArgs::results_t args(allocator);
	int err = cmd_args.parse(argc,argv,args);
	if (err	!= 0)
	{
		OOBase::LocalString strErr(allocator);
		if (args.find("missing",strErr))
			OOBase::Logger::log(OOBase::Logger::Error,APPNAME " - Missing value for option %s",strErr.c_str());
		else if (args.find("unknown",strErr))
			OOBase::Logger::log(OOBase::Logger::Error,APPNAME " - Unknown option %s",strErr.c_str());
		else
			OOBase::Logger::log(OOBase::Logger::Error,APPNAME " - Failed to parse command line: %s",OOBase::system_error_text(err));
			
		return EXIT_FAILURE;
	}
	
	s_is_debug = args.exists("debug");

	if (args.exists("help"))
		return Help();

	if (args.exists("version"))
		return Version();

#if defined(_WIN32)
	// If this event exists, then we are being debugged
	{
		// Scope it...
		OOBase::Win32::SmartHandle hDebugEvent(OpenEventW(EVENT_ALL_ACCESS,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX"));
		if (hDebugEvent)
		{
			// Wait for a bit, letting the caller attach a debugger
			WaitForSingleObject(hDebugEvent,15000);
		}
	}

#elif defined(HAVE_UNISTD_H) && 0

	if (s_is_debug)
	{
		printf("Attach a debugger to " APPNAME " process %u, and hit ENTER...\n",getpid());
		char buf[256];
		read(STDIN_FILENO,buf,sizeof(buf));
	}

#endif

	OOBase::LocalString strPipe(allocator);
	if (!args.find("pipe",strPipe))
	{
		// Oops...
		OOBase::Logger::log(OOBase::Logger::Error,APPNAME " - Invalid or missing arguments.");
		return EXIT_FAILURE;
	}

	return User::Manager().run(strPipe);
}
