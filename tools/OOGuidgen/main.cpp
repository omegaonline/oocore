///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
//
// This file is part of OOGuidgen, the Omega Online GUID generator application.
//
// OOGuidgen is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOGuidgen is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOGuidgen.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include <OOSvrBase/CmdArgs.h>

#include "../../include/Omega/Omega.h"

#include <stdio.h>

static int version()
{
	printf("ooguidgen version %s",OOCORE_VERSION);

#if defined(OMEGA_DEBUG)
	printf(" (Debug build)");
#endif
		
	return EXIT_SUCCESS;
}

static int help()
{
	printf("ooguidgen - The Omega Online GUID generator.\n\n");
	printf("Please consult the documentation at http://www.omegaonline.org.uk for further information.\n");
	
	return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
	// Set up the command line args
	OOSvrBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
	cmd_args.add_option("version",'v');
	
	// Parse command line
	OOSvrBase::CmdArgs::results_t args;
	if (cmd_args.parse(argc,argv,args) != 0)
		return EXIT_FAILURE;
		
	if (args.find("help") != args.npos)
		return help();

	if (args.find("version") != args.npos)
		return version();
		
	printf("%s",Omega::guid_t::Create().ToString().c_nstr());
	
	return EXIT_SUCCESS;
}
