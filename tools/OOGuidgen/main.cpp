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

#include <OOBase/CmdArgs.h>
#include <OOBase/StackAllocator.h>

#include "../../include/Omega/Omega.h"

#include <stdio.h>
#include <stdlib.h>

static int version()
{
	OOBase::stdout_write("ooguidgen version " OOCORE_VERSION);

#if !defined(NDEBUG)
	OOBase::stdout_write(" (Debug build)");
#endif

	return EXIT_SUCCESS;
}

static int help()
{
	OOBase::stdout_write(
			"ooguidgen - The Omega Online GUID generator.\n\n"
			"Please consult the documentation at http://www.omegaonline.org.uk for further information.\n");

	return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
	// Set up the command line args
	OOBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
	cmd_args.add_option("version",'v');

	// Parse command line
	OOBase::CmdArgs::results_t args;
	int err = cmd_args.parse(argc,argv,args);
	if (err	!= 0)
	{
		OOBase::String strErr;
		if (args.find("missing",strErr))
		{
			OOBase::stderr_write("Missing value for option ");
			OOBase::stderr_write(strErr.c_str());
		}
		else if (args.find("unknown",strErr))
		{
			OOBase::stderr_write("Unknown option ");
			OOBase::stderr_write(strErr.c_str());
		}
		else
		{
			OOBase::stderr_write("Failed to parse comand line: ");
			OOBase::stderr_write(OOBase::system_error_text(err));
		}
		return EXIT_FAILURE;
	}

	if (args.exists("help"))
		return help();

	if (args.exists("version"))
		return version();

	OOBase::stdout_write(Omega::guid_t::Create().ToString().c_str());
	OOBase::stdout_write("\n");

	return EXIT_SUCCESS;
}
