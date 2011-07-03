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

#include <OOBase/GlobalNew.h>
#include <OOBase/Singleton.h>
#include <OOBase/TLSSingleton.h>
#include <OOBase/SmartPtr.h>
#include <OOBase/CDRStream.h>
#include <OOBase/TimeVal.h>
#include <OOBase/Thread.h>
#include <OOBase/Socket.h>
#include <OOBase/BoundedQueue.h>
#include <OOBase/HandleTable.h>
#include <OOBase/Table.h>
#include <OOBase/DLL.h>
#include <OOBase/Win32.h>
#include <OOBase/utf8.h>
#include <OOBase/tr24731.h>

#include "../oocore-config.h"

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
// Include Omega/OTL components

// Ensure we have the local Omega.h instead of the installed one...
#include "../../include/Omega/Omega.h"
#include "../../include/OTL/OTL.h"
#include "../../include/OTL/Exception.h"
#include "../../include/OTL/Registry.h"

// End of Omega/OTL includes
/////////////////////////////////////////////////

/////////////////////////////////////////////////
// Some helpers and globals

#if defined(_WIN32)
#define OMEGA_THROW_NOMEM() OMEGA_THROW(ERROR_OUTOFMEMORY)
#else
#define OMEGA_THROW_NOMEM() OMEGA_THROW(ENOMEM)
#endif

namespace OOCore
{
	bool HostedByOOServer();
	
	struct DLL
	{
		int unused;
	};

	class OmegaFailure
	{
	public:
		static void fail()
		{
			OMEGA_THROW_NOMEM();
		}
	};

	struct GuidHash
	{
		static size_t hash(const Omega::guid_base_t& v)
		{
			size_t r = v.Data1;
			r ^= v.Data2;
			r ^= (v.Data3 << 16);
			r ^= (v.Data4[0] << 24);
			r ^= (v.Data4[1] << 16);
			r ^= (v.Data4[2] << 8);
			r ^= v.Data4[3];
			r ^= (v.Data4[4] << 24);
			r ^= (v.Data4[5] << 16);
			r ^= (v.Data4[6] << 8);
			r ^= v.Data4[7];
			return r;
		}
	};
}

#include "Formatting.h"

#endif // OOCORE_LOCAL_MACROS_H_INCLUDED_
