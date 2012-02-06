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
#include <OOBase/utf8.h>
#include <OOBase/SecurityWin32.h>
#include <OOBase/Logger.h>
#include <OOBase/Service.h>

#include <OOSvrBase/Proactor.h>

//////////////////////////////////////////////

#include "../oocore-config.h"

//////////////////////////////////////////////

/////////////////////////////////////////////////

// Ensure we have the local Omega.h instead of the installed one...
#include "../../include/Omega/Omega.h"
#include "../../include/Omega/Remoting.h"
#include "../../include/OTL/Exception.h"
#include "../../include/OTL/Registry.h"

#include "../../include/Omega/Service.h"

namespace User
{
	bool is_debug();
}

#endif // OOSERVER_USER_H_INCLUDED_
