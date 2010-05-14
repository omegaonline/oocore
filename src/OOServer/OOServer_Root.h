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

#include "../OOBase/Singleton.h"
#include "../OOBase/SmartPtr.h"
#include "../OOBase/TLSSingleton.h"
#include "../OOBase/CDRStream.h"
#include "../OOBase/pw_info.h"
#include "../OOBase/Queue.h"
#include "../OOBase/Thread.h"
#include "../OOBase/PosixSocket.h"

//////////////////////////////////////////////

#include "../OOBase/Proactor.h"
#include "../OOBase/Logger.h"
#include "../OOBase/CmdArgs.h"
#include "../OOBase/utf8.h"
#include "../OOBase/Database.h"

//////////////////////////////////////////////

#include "../include/Omega/internal/base_types.h"

//////////////////////////////////////////////

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
