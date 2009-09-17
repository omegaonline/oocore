///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOCORE_LOCAL_MACROS_H_INCLUDED_
#define OOCORE_LOCAL_MACROS_H_INCLUDED_

#include "../OOBase/Singleton.h"
#include "../OOBase/TLSSingleton.h"
#include "../OOBase/SmartPtr.h"
#include "../OOBase/CDRStream.h"
#include "../OOBase/TimeVal.h"
#include "../OOBase/Thread.h"
#include "../OOBase/Socket.h"
#include "../OOBase/Queue.h"
#include "../OOBase/DLL.h"
#include "../OOBase/Win32.h"
#include "../OOBase/utf8.h"
#include "../OOBase/tr24731.h"

//////////////////////////////////////////////
// Set up the export macros for OOCORE

#define OOCORE_INTERNAL

#define OOCORE_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_LOCAL_FUNCTION_VOID(name,param_count,params)

#define OOCORE_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_LOCAL_FUNCTION(ret_type,name,param_count,params)

#define OOCORE_RAW_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_LOCAL_FUNCTION_VOID(name,param_count,params)

#define OOCORE_RAW_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_LOCAL_FUNCTION(ret_type,name,param_count,params)

#define OOCORE_DECLARE_OID(name) \
	OMEGA_EXPORT_OID(name)

/////////////////////////////////////////////////
// Include OOCore/OTL components

#include <OTL/OTL.h>

// End of OOCore/OTL includes
/////////////////////////////////////////////////

/////////////////////////////////////////////////
// Some helpers and globals

namespace OOCore
{
	bool HostedByOOServer();

	struct DLL
	{
		int unused;
	};
}

#endif // OOCORE_LOCAL_MACROS_H_INCLUDED_
