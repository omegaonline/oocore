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
//  ***** THIS IS A SECURE MODULE *****
//
//  It will be run as Administrator/setuid root
//
//  Therefore it needs to be SAFE AS HOUSES!
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_ROOT_H_INCLUDED_
#define OOSERVER_ROOT_H_INCLUDED_

//////////////////////////////////////////////

#include <OOBase/GlobalNew.h>
#include <OOBase/Singleton.h>
#include <OOBase/SmartPtr.h>
#include <OOBase/TLSSingleton.h>
#include <OOBase/Environment.h>
#include <OOBase/CDRStream.h>
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
#include <OOBase/Proactor.h>

//////////////////////////////////////////////

#include "../oocore-config.h"

//////////////////////////////////////////////

#include "../../include/Omega/internal/base_types.h"


#if defined(_WIN32) && !defined(__MINGW32__)
#define APPNAME "OOServer"
#else
#define APPNAME "ooserverd"
#endif

#endif // OOSERVER_ROOT_H_INCLUDED_
