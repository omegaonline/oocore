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

#include "../OOBase/Singleton.h"
#include "../OOBase/SmartPtr.h"
#include "../OOBase/TLSSingleton.h"
#include "../OOBase/CDRStream.h"
#include "../OOBase/Queue.h"
#include "../OOBase/Thread.h"

//////////////////////////////////////////////

#include "../OOBase/Proactor.h"
#include "../OOBase/Logger.h"
#include "../OOBase/CmdArgs.h"

//////////////////////////////////////////////

#include <set>
#include <iostream>
#include <sstream>

/////////////////////////////////////////////////

#include <OTL/Remoting.h>
#include <OTL/Exception.h>
#include <OTL/Registry.h>

#include <OOCore/Service.h>

/////////////////////////////////////////////////

namespace User
{
	struct Module
	{
		int unused;
	};
}

#endif // OOSERVER_USER_H_INCLUDED_
