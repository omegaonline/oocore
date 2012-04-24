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

#include <stdlib.h>

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

#if defined(_MSC_VER)
// Shutup VS leak
extern "C" int _setenvp() { return 0; }
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

	bool CriticalFailure_early(const char* msg)
	{
		OOBase::stderr_write("Critical error in " APPNAME ":");
		OOBase::stderr_write(msg);
		OOBase::stderr_write("\n");

		if (Root::is_debug())
		{
			// Give us a chance to read the errors!
			OOBase::Thread::sleep(15000);
		}

		return true;
	}

	bool CriticalFailure(const char* msg)
	{
		OOBase::Logger::log(OOBase::Logger::Error,"%s",msg);

		if (Root::is_debug())
		{
			// Give us a chance to read the errors!
			OOBase::Thread::sleep(15000);
		}

		return true;
	}

	int Failure(const char* fmt, ...)
	{
		va_list args;
		va_start(args,fmt);

		OOBase::LocalString msg;
		int err = msg.vprintf(fmt,args);

		va_end(args);

		if (err == 0)
			OOBase::stderr_write(msg.c_str());

		return EXIT_FAILURE;
	}

	static bool s_is_debug = false;
}

bool Root::is_debug()
{
	return s_is_debug;
}

int main(int argc, const char* argv[])
{
	// Set critical failure handler
	OOBase::SetCriticalFailure(&CriticalFailure_early);

	// Get the debug ENV var
	{
		OOBase::LocalString str;
		str.getenv("OMEGA_DEBUG");
		s_is_debug = (str == "true");
	}

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
			return Failure("Missing value for option %s\n",strErr.c_str());
		else if (args.find("unknown",strErr))
			return Failure("Unknown option %s\n",strErr.c_str());
		else
			return Failure("Failed to parse command line: %s\n",OOBase::system_error_text(err));
	}

	if (args.exists("debug"))
		s_is_debug = true;

	if (args.exists("help"))
		return Help();

	if (args.exists("version"))
		return Version();

	OOBase::String strPidfile;
	args.find("pidfile",strPidfile);

	// Change this to OOServer::daemonize(strPidfile.c_str())
	void* TODO;

	err = OOBase::Server::pid_file(strPidfile.empty() ? "/var/run/" APPNAME ".pid" : strPidfile.c_str());
	if (err == EACCES)
		return Failure(APPNAME " is already running\n");
	else if (err)
		return Failure("Failed to create pid_file: %s\n",OOBase::system_error_text(err));

	// Start the logger - delayed because we may have forked
	OOBase::Logger::open("OOServer",__FILE__);

	// Set critical failure handler to the logging handler
	OOBase::SetCriticalFailure(&CriticalFailure);

	// Run the per-platform init routines
	if (!Root::platform_init())
		return EXIT_FAILURE;

	// Run the one and only Root::Manager instance
	err = Root::Manager().run(args);

#if defined(HAVE_UNISTD_H)

	void* TODO2; // Do something like use a destructing singleton to unlink()
	unlink(strPidfile.empty() ? "/var/run/" APPNAME ".pid" : strPidfile.c_str());

#endif

	return err;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Leave this function last because we include a lot of headers, which might be dangerous!

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
