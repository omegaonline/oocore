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

#ifndef OMEGA_H_INCLUDED_
#define OMEGA_H_INCLUDED_

//////////////////////////////////////////////
#include "internal/config-guess.h"
#include "OOCore_version.h"

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
#include <string>
#include <cmath>

#include <vector>
#include <deque>
#include <list>
#include <set>
#include <map>

// End of STL includes
//////////////////////////////////////////////

#if defined(_MSC_VER) && defined(_Wp64)
// MSVC moans about possibly poor size_t coercion - its checked - its all fine
#pragma warning(push)
#pragma warning(disable : 4244 4267)
#endif

#include "internal/memory.h"
#include "internal/types.h"
#include "internal/threading.h"
#include "internal/base.h"
#include "internal/export.h"
#include "internal/rtti.h"
#include "internal/any.h"
#include "internal/safe.h"
#include "internal/safe_ps.h"
#include "internal/wire.h"
#include "internal/wire_ps.h"
#include "internal/interfaces.h"

namespace Omega
{
	IException* Initialize() throw();
	void Uninitialize();

	IObject* CreateInstance(const any_t& oid, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid);
	bool_t HandleRequest(uint32_t timeout = 0);

	/* InitStandalone takes a map of key value pairs of type string_t (std::map<string_t,string_t>)
	 * Keys are:
	 *    regdb_path -        (Required) The directory containing the system and sandbox registry files
	 *    user_regdb -        (Required) The path to the current user's registry file, including filename
	 *    standalone_always - (Optional) If value = "true", ignore any running OOServer and only run standalone
	 */
	typedef std::map<string_t,string_t> init_arg_map_t;
	IException* InitStandalone(const init_arg_map_t& args) throw();
}

#include "internal/types.inl"
#include "internal/any.inl"
#include "internal/threading.inl"
#include "internal/rtti.inl"
#include "internal/safe.inl"
#include "internal/wire.inl"

#if defined(_MSC_VER) && defined(_Wp64)
#pragma warning(pop)
#endif

#if !defined(DOXYGEN)

OOCORE_EXPORTED_FUNCTION(Omega::IException*,OOCore_Omega_Initialize,0,())
inline Omega::IException* Omega::Initialize() throw()
{
	try
	{
#if !defined(OOCORE_INTERNAL)
		// Check the versions are correct
		Omega::uint32_t version = (OOCore::GetMajorVersion() << 24) | (OOCore::GetMinorVersion() << 16) | OOCore::GetPatchVersion();
		if (version < ((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16)))
			return Omega::IInternalException::Create("This component requires a later version of OOCore","Omega::Initialize");
#endif

		return OOCore_Omega_Initialize();
	}
	catch (Omega::IException* pE)
	{
		// Just in case...
		return pE;
	}
}

OOCORE_EXPORTED_FUNCTION(Omega::IException*,OOCore_Omega_InitStandalone,1,((in),const Omega::init_arg_map_t&,args))
inline Omega::IException* Omega::InitStandalone(const std::map<Omega::string_t,Omega::string_t>& args) throw()
{
	try
	{
#if !defined(OOCORE_INTERNAL)
		// Check the versions are correct
		Omega::uint32_t version = (OOCore::GetMajorVersion() << 24) | (OOCore::GetMinorVersion() << 16) | OOCore::GetPatchVersion();
		if (version < ((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16)))
			return Omega::IInternalException::Create("This component requires a later version of OOCore","Omega::Initialize");
#endif

		return OOCore_Omega_InitStandalone(args);
	}
	catch (Omega::IException* pE)
	{
		// Just in case...
		return pE;
	}
}

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_Omega_Uninitialize,0,())
inline void Omega::Uninitialize()
{
	OOCore_Omega_Uninitialize();
}

OOCORE_EXPORTED_FUNCTION(Omega::Activation::IObjectFactory*,OOCore_GetObjectFactory,2,((in),const Omega::any_t&,oid,(in),Omega::Activation::Flags_t,flags));
inline Omega::Activation::IObjectFactory* Omega::Activation::GetObjectFactory(const any_t& oid, Activation::Flags_t flags)
{
	return OOCore_GetObjectFactory(oid,flags);
}

inline Omega::IObject* Omega::CreateInstance(const any_t& oid, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid)
{
	System::Internal::auto_iface_ptr<Activation::IObjectFactory> ptrOF(OOCore_GetObjectFactory(oid,flags));

	IObject* pObject = 0;
	ptrOF->CreateInstance(pOuter,iid,pObject);
	return pObject;
}

OOCORE_EXPORTED_FUNCTION(Omega::bool_t,OOCore_Omega_HandleRequest,1,((in),Omega::uint32_t,timeout));
inline Omega::bool_t Omega::HandleRequest(uint32_t timeout)
{
	return OOCore_Omega_HandleRequest(timeout);
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_allocate,4,((in),size_t,len,(in),int,flags,(in),const char*,file,(in),unsigned int,line));
inline void* Omega::System::Allocate(size_t len, int flags, const char* file, unsigned int line)
{
	return OOCore_allocate(len,flags,file,line);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_free,2,((in),void*,mem,(in),int,flags));
inline void Omega::System::Free(void* mem, int flags)
{
	OOCore_free(mem,flags);
}

#endif // !defined(DOXYGEN)

#endif // OMEGA_H_INCLUDED_
