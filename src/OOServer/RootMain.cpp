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

static int Version();

static int Help()
{
	std::cout << "OOServer - The Omega Online network deamon." << std::endl << std::endl;
	std::cout << "Please consult the documentation at http://www.omegaonline.org.uk for further information." << std::endl << std::endl;

#if defined(_WIN32) && !defined(__MINGW32__)
	std::cout << "Usage: OOServer [options]" << std::endl << std::endl;
#else
	std::cout << "Usage: ooserverd [options]" << std::endl << std::endl;
#endif

	std::cout << "Options:" << std::endl;
	std::cout << "  --help (-h)      Display this help text" << std::endl;
	std::cout << "  --version (-v)   Display version information" << std::endl;
	std::cout << "  --install (-i)   Install" << std::endl;
	std::cout << "  --uninstall (-u) Uninstall"  << std::endl;
	std::cout << std::endl;

#if defined(_WIN32)
	std::cout << "If the install option is used, optionally append the sandbox username and password" << std::endl;
#else
	std::cout << "If the install option is used, append the sandbox username or uid" << std::endl;
#endif
	std::cout << std::endl;

	return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
	// Start the logger
	OOSvrBase::Logger::open("OOServer");

	// The one and only Root::Manager instance
	Root::Manager root_manager;

	// Set up the command line args
	OOSvrBase::CmdArgs cmd_args;
	cmd_args.add_option("install",'i',"install");
	cmd_args.add_option("uninstall",'u',"uninstall");
	cmd_args.add_option("help",'h',"help");
	cmd_args.add_option("version",'v',"version");

	// Parse command line
	std::map<std::string,std::string> args;
	if (!cmd_args.parse(argc,argv,args))
		return EXIT_FAILURE;

	if (args["help"] == "true")
		return Help();

	if (args["version"] == "true")
		return Version();
	
	if (args["install"] == "true")
	{
		if (!root_manager.install(args))
			return EXIT_FAILURE;

		std::cout << "Installed successfully." << std::endl;
		return EXIT_SUCCESS;
	}

	if (args["uninstall"] == "true")
	{
		if (!root_manager.uninstall())
			return EXIT_FAILURE;

		std::cout << "Uninstalled successfully." << std::endl;
		return EXIT_SUCCESS;
	}

	// Run the RootManager
	return root_manager.run();
}

///////////////////////////////////////////////////////////////////////////////////////////
// Leave this function last because we includes a lot of headers, which might be dangerous!

#include <OOCore/config-guess.h>
#include <OOCore/version.h>
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
