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

#ifndef OOCORE_H_INCLUDED_
#define OOCORE_H_INCLUDED_

//////////////////////////////////////////////
#include "config-guess.h"

#if defined(_WIN32)
// MS define interface as well...
#include <objbase.h>
#endif

//////////////////////////////////////////////
// Set up the export macros for OOCORE
#if defined(DOXYGEN)

#define OMEGA_DECLARE_OID(object_identifier) \
	const Omega::guid_t object_identifier

#elif !defined(OMEGA_EXPORTED_FUNCTION_VOID)

#define OMEGA_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_EXPORTED_FUNCTION_VOID_IMPL(name,param_count,params)

#define OMEGA_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_EXPORTED_FUNCTION_IMPL(ret_type,name,param_count,params)

#define OMEGA_DECLARE_OID(n) \
	OMEGA_IMPORT_OID(n)

#endif

//////////////////////////////////////////////
// Include STL components

#include <set>
#include <list>
#include <map>
#include <vector>
#include <string>

// End of STL includes
//////////////////////////////////////////////

#include "Types.h"
#include "Threading.h"
#include "Base.h"
#include "Export.h"
#include "Rtti.h"
#include "Wire.h"
#include "Interfaces.h"

namespace Omega
{
	inline IException* Initialize();
	inline void Uninitialize();

	inline IObject* CreateLocalInstance(const guid_t& oid, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid);
	inline IObject* CreateInstance(const string_t& strURI, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid);
	inline bool_t HandleRequest(uint32_t timeout = 0);

	namespace System
	{
		inline string_t GetVersion();
	}
}

#if !defined(DOXYGEN)

OMEGA_EXPORTED_FUNCTION(Omega::IException*,Omega_Initialize,0,())
Omega::IException* Omega::Initialize()
{
	return Omega_Initialize();
}

OMEGA_EXPORTED_FUNCTION_VOID(Omega_Uninitialize,0,())
void Omega::Uninitialize()
{
	Omega_Uninitialize();
}

OMEGA_EXPORTED_FUNCTION_VOID(Omega_CreateLocalInstance,5,((in),const Omega::guid_t&,oid,(in),Omega::Activation::Flags_t,flags,(in),Omega::IObject*,pOuter,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject));
Omega::IObject* Omega::CreateLocalInstance(const guid_t& oid, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid)
{
	IObject* pObj = 0;
	Omega_CreateLocalInstance(oid,flags,pOuter,iid,pObj);
	return pObj;
}

OMEGA_EXPORTED_FUNCTION_VOID(Omega_CreateInstance,5,((in),const Omega::string_t&,strURI,(in),Omega::Activation::Flags_t,flags,(in),Omega::IObject*,pOuter,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject));
Omega::IObject* Omega::CreateInstance(const string_t& strURI, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid)
{
	IObject* pObj = 0;
	Omega_CreateInstance(strURI,flags,pOuter,iid,pObj);
	return pObj;
}

OMEGA_EXPORTED_FUNCTION(Omega::bool_t,Omega_HandleRequest,1,((in),Omega::uint32_t,timeout));
Omega::bool_t Omega::HandleRequest(uint32_t timeout)
{
	return Omega_HandleRequest(timeout);
}

OMEGA_EXPORTED_FUNCTION(Omega::string_t,Omega_GetVersion,0,())
Omega::string_t Omega::System::GetVersion()
{
	return Omega_GetVersion();
}

#endif // !defined(DOXYGEN)

#include "Types.inl"
#include "Threading.inl"
#include "Rtti.inl"

#endif // OOCORE_H_INCLUDED_
