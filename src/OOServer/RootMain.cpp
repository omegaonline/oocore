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

#if defined(_WIN32)
#include <shlwapi.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <sys/stat.h>
#endif

#ifdef HAVE_VLD_H
#include <vld.h>
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
		return true;
	}

	bool CriticalFailure(const char* msg)
	{
		OOBase::Logger::log(OOBase::Logger::Error,"%s",msg);
		return true;
	}

	int Failure(OOBase::AllocatorInstance& allocator, const char* fmt, ...)
	{
		va_list args;
		va_start(args,fmt);

		OOBase::ScopedArrayPtr<char,OOBase::AllocatorInstance> msg(allocator);
		int err = OOBase::temp_vprintf(msg,fmt,args);

		va_end(args);

		if (err == 0)
			OOBase::stderr_write(msg.get());

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

	// Declare a local stack allocator
	OOBase::StackAllocator<1024> allocator;

	// Get the debug ENV var
	{
		OOBase::ScopedArrayPtr<char> str;
		OOBase::Environment::getenv("OMEGA_DEBUG",str);
		s_is_debug = (strcmp(str.get(),"true") == 0);
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
			return Failure(allocator,"Missing value for option %s\n",strErr.c_str());
		else if (args.find("unknown",strErr))
			return Failure(allocator,"Unknown option %s\n",strErr.c_str());
		else
			return Failure(allocator,"Failed to parse command line: %s\n",OOBase::system_error_text(err));
	}

	if (args.exists("debug"))
		s_is_debug = true;

	if (args.exists("help"))
		return Help();

	if (args.exists("version"))
		return Version();

	OOBase::String strPidfile;
	if (!args.find("pidfile",strPidfile))
	{
		err = strPidfile.assign("/var/run/" APPNAME ".pid");
		if (err)
			return Failure(allocator,"Failed to assign string: %s\n",OOBase::system_error_text(err));
	}

	// Daemonize if not debug
	bool already_running = false;
	if (!s_is_debug)
		err = OOBase::Server::daemonize(strPidfile.c_str(),already_running);
	else
		err = OOBase::Server::create_pid_file(strPidfile.c_str(),already_running);

	if (err)
		return Failure(allocator,"Failed to create pid_file: %s\n",OOBase::system_error_text(err));
	else if (already_running)
		return Failure(allocator,APPNAME " is already running\n");

	// Start the logger - delayed because we may have forked
	OOBase::Logger::open_system_log("OOServer",__FILE__);

	// Set critical failure handler to the logging handler
	OOBase::SetCriticalFailure(&CriticalFailure);

#if defined(_WIN32)
	if (!s_is_debug)
	{
		// Change working directory to the location of the executable (we know it's valid!)
		wchar_t szPath[MAX_PATH];
		if (!GetModuleFileNameW(NULL,szPath,MAX_PATH))
			LOG_ERROR_RETURN(("GetModuleFileName failed: %s",OOBase::system_error_text()),EXIT_FAILURE);

		// Strip off our name
		PathUnquoteSpacesW(szPath);
		PathRemoveFileSpecW(szPath);

		if (!SetCurrentDirectoryW(szPath))
			LOG_ERROR_RETURN(("SetCurrentDirectory(%ls) failed: %s",szPath,OOBase::system_error_text()),EXIT_FAILURE);
	}
#elif defined(HAVE_UNISTD_H)

	// Ignore SIGCHLD and SIGPIPE
	sigset_t sigset;
	err = sigemptyset(&sigset);
	if (!err)
		err = sigaddset(&sigset, SIGCHLD);
	if (!err)
		err = sigaddset(&sigset, SIGPIPE);
	if (err)
		err = errno;
	else
		err = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	if (err)
		LOG_ERROR_RETURN(("Failed to adjust signals: %s",OOBase::system_error_text(err)),EXIT_FAILURE);

	umask(0);

	// Change working directory to a known location
	if (!s_is_debug && chdir("/") != 0)
		LOG_ERROR_RETURN(("Failed to change current directory to /: %s",OOBase::system_error_text()),EXIT_FAILURE);
#endif

	// Run the one and only Root::Manager instance
	return Root::Manager().run(args);
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
		return EXIT_SUCCESS;
	}
}
