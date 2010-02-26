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

//////////////////////////////////////////////
// Set up the correct export macros

#if !defined(OOCORE_INTERNAL) 

#if !defined(DOXYGEN)

#define OOCORE_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_EXPORTED_FUNCTION_VOID_IMPL(name,param_count,params)

#define OOCORE_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_EXPORTED_FUNCTION_IMPL(ret_type,name,param_count,params)

#define OOCORE_RAW_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_RAW_EXPORTED_FUNCTION_VOID_IMPL(name,param_count,params)

#define OOCORE_RAW_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_RAW_EXPORTED_FUNCTION_IMPL(ret_type,name,param_count,params)

#endif // !DOXYGEN

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

#include <limits>
#include <set>
#include <list>
#include <map>
#include <vector>
#include <string>

// End of STL includes
//////////////////////////////////////////////

#if defined(_MSC_VER) && defined(_Wp64)
// MSVC moans about possibly poor size_t coercion - its checked - its all fine
#pragma warning(push)
#pragma warning(disable : 4244 4267)
#endif

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
	IException* Initialize(bool bStandalone = false);
	void Uninitialize();

	IObject* CreateLocalInstance(const guid_t& oid, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid);
	IObject* CreateInstance(const string_t& strURI, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid);
	bool_t HandleRequest(uint32_t timeout = 0);
}

#include "Types.inl"
#include "Threading.inl"
#include "Rtti.inl"
#include "Safe.inl"
#include "Wire.inl"

#if defined(_MSC_VER) && defined(_Wp64)
#pragma warning(pop)
#endif

#if !defined(DOXYGEN)

OOCORE_EXPORTED_FUNCTION(Omega::IException*,OOCore_Omega_Initialize,1,((in),Omega::bool_t,bStandalone))
inline Omega::IException* Omega::Initialize(bool bStandalone)
{
#if !defined(OOCORE_INTERNAL)
	// Check the versions are correct
	if (OOCore::GetMajorVersion() < OOCORE_MAJOR_VERSION)
		OMEGA_THROW(L"This component requires a later version of OOCore");
#endif

	return OOCore_Omega_Initialize(bStandalone);
}

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_Omega_Uninitialize,0,())
inline void Omega::Uninitialize()
{
	OOCore_Omega_Uninitialize();
}

inline Omega::IObject* Omega::CreateLocalInstance(const guid_t& oid, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid)
{
	System::Internal::auto_iface_ptr<Activation::IObjectFactory> ptrOF(static_cast<Activation::IObjectFactory*>(Omega::Activation::GetRegisteredObject(oid,flags,OMEGA_GUIDOF(Activation::IObjectFactory))));
	if (!ptrOF)
		OMEGA_THROW(L"Failed to create object factory");
	
	IObject* pObject = 0;
	ptrOF->CreateInstance(pOuter,iid,pObject);
	return pObject;
}

OOCORE_EXPORTED_FUNCTION(Omega::Activation::IObjectFactory*,OOCore_GetObjectFactory,2,((in),const Omega::string_t&,strURI,(in),Omega::Activation::Flags_t,flags));
inline Omega::IObject* Omega::CreateInstance(const string_t& strURI, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid)
{
	System::Internal::auto_iface_ptr<Activation::IObjectFactory> ptrOF(OOCore_GetObjectFactory(strURI,flags));
	if (!ptrOF)
		OMEGA_THROW(L"Failed to create object factory");
	
	IObject* pObject = 0;
	ptrOF->CreateInstance(pOuter,iid,pObject);
	return pObject;
}

OOCORE_EXPORTED_FUNCTION(Omega::bool_t,OOCore_Omega_HandleRequest,1,((in),Omega::uint32_t,timeout));
inline Omega::bool_t Omega::HandleRequest(uint32_t timeout)
{
	return OOCore_Omega_HandleRequest(timeout);
}

#endif // !defined(DOXYGEN)

#endif // OOCORE_H_INCLUDED_
