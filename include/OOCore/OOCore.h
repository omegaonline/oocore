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
#include "version.h"

#if defined(_WIN32)
// MS define interface as well...
#include <objbase.h>
#endif

//////////////////////////////////////////////
// Set up the correct export macros

#if !defined(OOCORE_INTERNAL)

#define OOCORE_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_EXPORTED_FUNCTION_VOID_IMPL(name,param_count,params)

#define OOCORE_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_EXPORTED_FUNCTION_IMPL(ret_type,name,param_count,params)

#define OOCORE_RAW_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_RAW_EXPORTED_FUNCTION_VOID_IMPL(name,param_count,params)

#define OOCORE_RAW_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_RAW_EXPORTED_FUNCTION_IMPL(ret_type,name,param_count,params)

#define OOCORE_DECLARE_OID(name) \
	OMEGA_IMPORT_OID(name)

#endif

//////////////////////////////////////////////
// Set up the correct private macros

#if !defined(OMEGA_MODULE_PRIVATE_NAME)
#define OMEGA_MODULE_PRIVATE_NAME hidden_
#endif

#define OMEGA_PRIVATE_TYPE(ty)      OMEGA_CONCAT(OMEGA_MODULE_PRIVATE_NAME,ty)
#define OMEGA_PRIVATE_FN_DECL(r,fn) r OMEGA_CONCAT(OMEGA_MODULE_PRIVATE_NAME,fn)
#define OMEGA_PRIVATE_FN_CALL(fn)   OMEGA_CONCAT(OMEGA_MODULE_PRIVATE_NAME,fn)

#if !defined(OMEGA_WEAK_VARIABLE)
#define OMEGA_WEAK_VARIABLE(t,v) \
	static const t v;
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
#include "Safe.h"
#include "SafePS.h"
#include "Wire.h"
#include "WirePS.h"
#include "Interfaces.h"

namespace Omega
{
	inline IException* Initialize(bool bStandalone = false);
	inline void Uninitialize();

	inline IObject* CreateLocalInstance(const guid_t& oid, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid);
	inline IObject* CreateInstance(const string_t& strURI, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid);
	inline bool_t HandleRequest(uint32_t timeout = 0);
}

#include "Types.inl"
#include "Threading.inl"
#include "Rtti.inl"
#include "Safe.inl"
#include "Wire.inl"

#if !defined(DOXYGEN)

OOCORE_EXPORTED_FUNCTION(Omega::IException*,OOCore_Omega_Initialize,1,((in),Omega::bool_t,bStandalone))
Omega::IException* Omega::Initialize(bool bStandalone)
{
#if !defined(OOCORE_INTERNAL)
	// Check the versions are correct
	if (OOCore::GetMajorVersion() < OOCORE_MAJOR_VERSION)
		OMEGA_THROW(L"This component requires a later version of OOCore");
#endif

	return OOCore_Omega_Initialize(bStandalone);
}

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_Omega_Uninitialize,0,())
void Omega::Uninitialize()
{
	OOCore_Omega_Uninitialize();
}

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_Omega_CreateLocalInstance,5,((in),const Omega::guid_t&,oid,(in),Omega::Activation::Flags_t,flags,(in),Omega::IObject*,pOuter,(in),const Omega::guid_t&,iid,(out)(iid_is(iid))(outer_is(pOuter)),Omega::IObject*&,pObject));
Omega::IObject* Omega::CreateLocalInstance(const guid_t& oid, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid)
{
	IObject* pObj = 0;
	OOCore_Omega_CreateLocalInstance(oid,flags,pOuter,iid,pObj);
	return pObj;
}

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_Omega_CreateInstance,5,((in),const Omega::string_t&,strURI,(in),Omega::Activation::Flags_t,flags,(in),Omega::IObject*,pOuter,(in),const Omega::guid_t&,iid,(out)(iid_is(iid))(outer_is(pOuter)),Omega::IObject*&,pObject));
Omega::IObject* Omega::CreateInstance(const string_t& strURI, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid)
{
	IObject* pObj = 0;
	OOCore_Omega_CreateInstance(strURI,flags,pOuter,iid,pObj);
	return pObj;
}

OOCORE_EXPORTED_FUNCTION(Omega::bool_t,OOCore_Omega_HandleRequest,1,((in),Omega::uint32_t,timeout));
Omega::bool_t Omega::HandleRequest(uint32_t timeout)
{
	return OOCore_Omega_HandleRequest(timeout);
}

#endif // !defined(DOXYGEN)

#endif // OOCORE_H_INCLUDED_
