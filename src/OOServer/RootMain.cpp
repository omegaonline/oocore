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
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "RootManager.h"

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#define APPNAME "OOServer"
#else
#define APPNAME "ooserverd"
#endif

static int Version();

static int Help()
{
	std::cout << APPNAME " - The Omega Online network deamon." << std::endl;
	std::cout << std::endl;
	std::cout << "Please consult the documentation at http://www.omegaonline.org.uk for further information." << std::endl;
	std::cout << std::endl;
	std::cout << "Usage: " APPNAME " [options]" << std::endl;
	std::cout << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "  --help (-h)      Display this help text" << std::endl;
	std::cout << "  --version (-v)   Display version information" << std::endl;
	std::cout << std::endl;
	std::cout << "  --conf-file (-f) <file_path> Use the specified configuration file" << std::endl;
	
	return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
	// Start the logger
	OOSvrBase::Logger::open("OOServer");

	// Set up the command line args
	OOSvrBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
	cmd_args.add_option("version",'v');
	cmd_args.add_option("conf-file",'f',true);
	cmd_args.add_option("unsafe",0);
	
	// Parse command line
	std::map<std::string,std::string> args;
	if (!cmd_args.parse(argc,argv,args))
		return EXIT_FAILURE;

	if (args.find("help") != args.end())
		return Help();

	if (args.find("version") != args.end())
		return Version();

	// Run the one and only Root::Manager instance
	return Root::Manager(args).run();
}

///////////////////////////////////////////////////////////////////////////////////////////
// Leave this function last because we includes a lot of headers, which might be dangerous!

#include "../include/Omega/internal/config-guess.h"
#include "../include/Omega/version.h"
#include <sqlite3.h>

static int Version()
{
	std::cout << "OOServer version information:" << std::endl;
#if defined(OMEGA_DEBUG)
	std::cout << "Version: " << OOCORE_VERSION << " (Debug build)" << std::endl;
#else
	std::cout << "Version: " << OOCORE_VERSION << std::endl;
#endif
	std::cout << "Compiler: " << OMEGA_COMPILER_STRING << std::endl;
	std::cout << "SQLite library version: " << sqlite3_libversion() << ", built with " << sqlite3_version << " headers" << std::endl;
	std::cout << std::endl;

	return EXIT_SUCCESS;
}
