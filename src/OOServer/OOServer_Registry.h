///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2013 Rick Taylor
//
// This file is part of OOSvrReg, the Omega Online registry server.
//
// OOSvrReg is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrReg is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOSvrReg.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOSERVER_REGISTRY_H_INCLUDED_
#define OOSERVER_REGISTRY_H_INCLUDED_

//////////////////////////////////////////////

#include <OOBase/GlobalNew.h>
#include <OOBase/Singleton.h>
#include <OOBase/SmartPtr.h>
#include <OOBase/TLSSingleton.h>
#include <OOBase/Environment.h>
#include <OOBase/Posix.h>
#include <OOBase/BoundedQueue.h>
#include <OOBase/StackAllocator.h>
#include <OOBase/HandleTable.h>
#include <OOBase/Thread.h>
#include <OOBase/Socket.h>
#include <OOBase/Set.h>
#include <OOBase/String.h>
#include <OOBase/CmdArgs.h>
#include <OOBase/Win32Security.h>
#include <OOBase/Logger.h>
#include <OOBase/Server.h>
#include <OOBase/ConfigFile.h>
#include <OOBase/CDRIO.h>

//////////////////////////////////////////////

#include "../oocore-config.h"

//////////////////////////////////////////////

#if defined(_WIN32) && !defined(__MINGW32__)
#define APPNAME "OOSvrReg"
#else
#define APPNAME "oosvrreg"
#endif

namespace Registry
{
	bool is_debug();
}

#if defined(_WIN32)
typedef OOBase::Win32::SmartHandle uid_t;
typedef DWORD pid_t;
#endif

#endif // OOSERVER_REGISTRY_H_INCLUDED_
