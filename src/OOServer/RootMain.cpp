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
//  ***** THIS IS A SECURE MODULE *****
//
//  It will be run as Administrator/setuid root
//
//  Therefore it needs to be SAFE AS HOUSES!
//
//  Do not include anything unnecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"

#include "RootManager.h"

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
	int Version();

	int Help()
	{
		OOBase::stdout_write(APPNAME " - The Omega Online network deamon.\n\n"
			"Please consult the documentation at http://www.omegaonline.org.uk for further information.\n\n"
			"Usage: " APPNAME " [options]\n\n"
			"Options:\n"
			"  --help (-h)      Display this help text\n"
			"  --version (-v)   Display version information\n"
			"\n"
			"  --conf-file (-f) <file_path>  Use the specified configuration file\n"
			"  --pidfile <file_path>         Override the default name and location for the pidfile\n"
			"\n");

		return EXIT_SUCCESS;
	}

	bool CriticalFailure(const char* msg)
	{
		OOBase::Logger::log(OOBase::Logger::Error,msg);

		if (Root::is_debug())
		{
			// Give us a chance to read the errors!
			OOBase::Thread::sleep(OOBase::timeval_t(15));
		}

		return true;
	}

	static bool s_is_debug = false;
}

bool Root::is_debug()
{
	return s_is_debug;
}

int main(int argc, char* argv[])
{
	// Get the debug ENV var
	{
		OOBase::LocalString str;
		str.getenv("OMEGA_DEBUG");
		s_is_debug = (str == "true");
	}

	// Start the logger
	OOBase::Logger::open("OOServer",__FILE__);

	// Set critical failure handler
	OOBase::SetCriticalFailure(&CriticalFailure);

	// Set up the command line args
	OOBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
	cmd_args.add_option("version",'v');
	cmd_args.add_option("conf-file",'f',true);
	cmd_args.add_option("pidfile",0,true);
	cmd_args.add_option("debug");

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

	if (args.exists("debug"))
		s_is_debug = true;

	if (args.exists("help"))
		return Help();

	if (args.exists("version"))
		return Version();

#if !defined(_WIN32)
	// Ignore SIGCHLD
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);

	umask(0);
#endif

	// Run the one and only Root::Manager instance
	return Root::Manager().run(args);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Leave this function last because we includes a lot of headers, which might be dangerous!

#include "../../include/Omega/internal/config-guess.h"
#include "../../include/Omega/OOCore_version.h"

namespace
{
	int Version()
	{
		OOBase::stdout_write(APPNAME " version " OOCORE_VERSION);

	#if !defined(NDEBUG)
		OOBase::stdout_write(" (Debug build)");
	#endif

		OOBase::stdout_write("\n\tCompiler: " OMEGA_COMPILER_STRING "\n");

		OOBase::stdout_write("\tSQLite library version: ");
		OOBase::stdout_write(sqlite3_libversion());
		OOBase::stdout_write(", built with ");
		OOBase::stdout_write(sqlite3_version);
		OOBase::stdout_write(" headers\n");

		return EXIT_SUCCESS;
	}
}
