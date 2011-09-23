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

namespace
{
	int Version();

	int Help()
	{
		printf(APPNAME " - The Omega Online network deamon.\n\n"
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
		OOSvrBase::Logger::log(OOSvrBase::Logger::Error,msg);

#if defined(OMEGA_DEBUG)
		// Give us a chance to read the errors!
		OOBase::Thread::sleep(OOBase::timeval_t(15));
#endif
		return true;
	}
}

int main(int argc, char* argv[])
{
	// Start the logger
	OOSvrBase::Logger::open("OOServer",__FILE__);

	// Set critical failure handler
	OOBase::SetCriticalFailure(&CriticalFailure);

	// Set up the command line args
	OOBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
	cmd_args.add_option("version",'v');
	cmd_args.add_option("conf-file",'f',true);
	cmd_args.add_option("pidfile",0,true);
	cmd_args.add_option("unsafe");

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

#if !defined(_WIN32)
	// Ignore  SIGCHLD
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);
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
		printf(APPNAME " version %s",OOCORE_VERSION);

	#if defined(OMEGA_DEBUG)
		printf(" (Debug build)");
	#endif

		printf("\n\tCompiler: %s\n",OMEGA_COMPILER_STRING);
		printf("\tSQLite library version: %s, built with %s headers\n",sqlite3_libversion(),sqlite3_version);

		return EXIT_SUCCESS;
	}
}
