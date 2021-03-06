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

#ifndef OOSERVER_USER_H_INCLUDED_
#define OOSERVER_USER_H_INCLUDED_

//////////////////////////////////////////////

#include <OOBase/GlobalNew.h>
#include <OOBase/CmdArgs.h>
#include <OOBase/Singleton.h>
#include <OOBase/SmartPtr.h>
#include <OOBase/TLSSingleton.h>
#include <OOBase/CDRStream.h>
#include <OOBase/Posix.h>
#include <OOBase/BoundedQueue.h>
#include <OOBase/HandleTable.h>
#include <OOBase/Thread.h>
#include <OOBase/Set.h>
#include <OOBase/Environment.h>
#include <OOBase/Win32Security.h>
#include <OOBase/Logger.h>
#include <OOBase/Server.h>
#include <OOBase/StackAllocator.h>
#include <OOBase/Proactor.h>

//////////////////////////////////////////////

#include "../oocore-config.h"

//////////////////////////////////////////////

#if defined(_WIN32)
	#define APPNAME "OOSvrUser"
#else
	#define APPNAME "oosvruser"
#endif

/////////////////////////////////////////////////

// Ensure we have the local Omega.h instead of the installed one...
#include "../../include/Omega/Omega.h"
#include "../../include/Omega/Remoting.h"
#include "../../include/Omega/Service.h"
#include "../../include/OTL/Exception.h"
#include "../../include/OTL/Registry.h"

#include "../../include/Omega/Service.h"

#include "../OOCore/Server.h"

#define OMEGA_CREATE_INTERNAL(e)  (Omega::System::Internal::auto_iface_ptr<Omega::IInternalException>(Omega::IInternalException::Create(Omega::string_t::constant(e),__FILE__,__LINE__,OMEGA_FUNCNAME)))

namespace User
{
	bool is_debug();

	Omega::string_t recurse_log_exception(Omega::IException* pE);
}

#endif // OOSERVER_USER_H_INCLUDED_
