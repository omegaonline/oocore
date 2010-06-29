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

#include <oobase/Singleton.h>
#include <oobase/SmartPtr.h>
#include <oobase/TLSSingleton.h>
#include <oobase/CDRStream.h>
#include <oobase/pw_info.h>
#include <oobase/Queue.h>
#include <oobase/Thread.h>
#include <oobase/PosixSocket.h>
#include <oobase/Proactor.h>
#include <oobase/Logger.h>
#include <oobase/CmdArgs.h>
#include <oobase/utf8.h>
#include <oobase/Database.h>
#include <oobase/SecurityWin32.h>
#include <oobase/Win32Socket.h>

//////////////////////////////////////////////

#include "../include/Omega/internal/base_types.h"

//////////////////////////////////////////////

#include <algorithm>
#include <map>
#include <set>
#include <iostream>
#include <sstream>
#include <fstream>

//////////////////////////////////////////////

namespace Root
{
	struct Module
	{
		int unused;
	};
}

#endif // OOSERVER_ROOT_H_INCLUDED_
