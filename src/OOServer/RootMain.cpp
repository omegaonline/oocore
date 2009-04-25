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
#include "../Common/Version.h"

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

static int Version();

static int Help()
{
	printf("OOServer - The Omega Online network deamon.\n\n");
	printf("Please consult the documentation at http://www.omegaonline.org.uk for further information.\n\n");
	return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
	// Start the logger
	OOSvrBase::Logger::open("OOServer");

	// The one and only Root::Manager instance
	Root::Manager root_manager;

	// We do the most basic command line parsing...
	if (argc > 1)
	{
		if (strcmp(argv[1],"--install")==0)
		{
			if (!root_manager.install(argc-2,&argv[2]))
				return EXIT_FAILURE;

			printf("Installed successfully.\n");
			return EXIT_SUCCESS;
		}
		else if (strcmp(argv[1],"--uninstall")==0)
		{
			if (!root_manager.uninstall())
				return EXIT_FAILURE;

			printf("Uninstalled successfully.\n");
			return EXIT_SUCCESS;
		}
		else if (strcmp(argv[1],"--helpstring")==0 || strcmp(argv[1],"/?")==0)
		{
			return Help();
		}
		else if (strcmp(argv[1],"--version")==0)
		{
			return Version();
		}
	}

	// Run the RootManager
	int ret = EXIT_FAILURE;
	ret = root_manager.run(argc,argv);

	// Make sure all the singletons etc have been destroyed
	OOBase::Destructor::call_destructors();

	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Leave this function last because we includes a lot of headers, which might be dangerous!
#include <OOCore/OOCore.h>
#include <sqlite3.h>
static int Version()
{
	printf("OOServer version information:\n");
#if defined(OMEGA_DEBUG)
	printf("Version: %s (Debug build)\nPlatform: %s\nCompiler: %s\n",OOCORE_VERSION,OMEGA_PLATFORM_STRING,OMEGA_COMPILER_STRING);
#else
	printf("Version: %s\nPlatform: %s\nCompiler: %s\n",OOCORE_VERSION,OMEGA_PLATFORM_STRING,OMEGA_COMPILER_STRING);
#endif

	printf("\nOOCore version information:\n");
	printf("%ls\n\n",Omega::System::GetVersion().c_str());

	printf("SQLite version: %s\n",SQLITE_VERSION);

	printf("\n");
	return EXIT_SUCCESS;
}
