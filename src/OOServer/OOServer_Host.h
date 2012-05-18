///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2012 Rick Taylor
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

#ifndef OOSERVER_HOST_H_INCLUDED_
#define OOSERVER_HOST_H_INCLUDED_

//////////////////////////////////////////////

#include <OOBase/GlobalNew.h>
#include <OOBase/CmdArgs.h>
//#include <OOBase/Singleton.h>
//#include <OOBase/SmartPtr.h>
//#include <OOBase/TLSSingleton.h>
//#include <OOBase/CDRStream.h>
//#include <OOBase/Posix.h>
//#include <OOBase/BoundedQueue.h>
//#include <OOBase/HandleTable.h>
#include <OOBase/Thread.h>
//#include <OOBase/Set.h>
//#include <OOBase/Environment.h>
//#include <OOBase/SecurityWin32.h>
#include <OOBase/Logger.h>
//#include <OOBase/Server.h>

//#include <OOSvrBase/Proactor.h>

//////////////////////////////////////////////

#include "../oocore-config.h"

//////////////////////////////////////////////

#if defined(_WIN32) && !defined(__MINGW32__)
	#define APPNAME "OOSvrHost"
#else
	#define APPNAME "oosvrhost"
#endif

/////////////////////////////////////////////////

// Ensure we have the local Omega.h instead of the installed one...
#include "../../include/Omega/Omega.h"
#include "../../include/Omega/Remoting.h"

#include "../../include/OTL/OTL.h"

//#include "../../include/Omega/Service.h"

#include "../OOCore/Server.h"

#define OMEGA_CREATE_INTERNAL(e)  (Omega::System::Internal::auto_iface_ptr<Omega::IInternalException>(Omega::IInternalException::Create(Omega::string_t::constant(e),__FILE__,__LINE__,OMEGA_FUNCNAME)))

namespace Host
{
	bool is_debug();

	int SingleSurrogate();
	int MultipleSurrogate();
}

#endif // OOSERVER_HOST_H_INCLUDED_
