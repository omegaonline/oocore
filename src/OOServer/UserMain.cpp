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
#include "../Common/Version.h"

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

int main(int argc, char* argv[])
{
	// Start the logger - use OOServer again...
	OOSvrBase::Logger::open("OOServer");

	// Set up the command line args
	OOSvrBase::CmdArgs cmd_args;
	cmd_args.add_argument("port",0);
	
	// Parse command line
	std::map<std::string,std::string> args;
	if (!cmd_args.parse(argc,argv,args))
		return EXIT_FAILURE;
	
#if defined(_WIN32) && defined(OMEGA_DEBUG)
	// If this event exists, then we are being debugged
	// Scope it...
	{
		OOBase::Win32::SmartHandle hDebugEvent(OpenEventW(EVENT_ALL_ACCESS,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX"));
		if (hDebugEvent)
		{
			// Wait for a bit, letting the caller attach a debugger
			WaitForSingleObject(hDebugEvent,5000);
		}
	}

#endif

	// Run the UserManager
	return User::Manager::run(args["port"]);
}

namespace OOBase
{
	// This is the critical failure hook
	void CriticalFailure(const char* msg)
	{
		std::cerr << msg << std::endl << std::endl;
		std::cerr << "Aborting!"  << std::endl;
	}
}
