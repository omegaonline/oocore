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
#include "./internal/config-guess.h"
#include "./OOCore_version.h"

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

#include "./internal/memory.h"
#include "./internal/types.h"
#include "./internal/threading.h"
#include "./internal/base.h"
#include "./internal/export.h"
#include "./internal/rtti.h"
#include "./internal/any.h"
#include "./internal/safe.h"
#include "./internal/safe_ps.h"
#include "./internal/wire.h"
#include "./internal/wire_ps.h"

namespace Omega
{
	IException* Initialize();
	void Uninitialize();

	interface ISystemException : public IException
	{
		virtual uint32_t GetErrorCode() = 0;

		static ISystemException* Create(uint32_t errno_val, IException* pCause = NULL);
	};

	interface ITimeoutException : public IException
	{
		static ITimeoutException* Create();
	};

	interface INotFoundException : public IException
	{
		static INotFoundException* Create(const string_t& strDesc, IException* pCause = NULL);
	};

	interface IAlreadyExistsException : public IException
	{
		static IAlreadyExistsException* Create(const string_t& strDesc);
	};

	interface IAccessDeniedException : public IException
	{
		static IAccessDeniedException* Create(const string_t& strDesc, IException* pCause = NULL);
	};

	interface ICastException : public IException
	{
		virtual any_t GetValue() = 0;
		virtual any_t::CastResult_t GetReason() = 0;
		virtual Remoting::IMessage* GetDestinationType() = 0;
	};

	namespace Activation
	{
		interface IObjectFactory : public IObject
		{
			virtual void CreateInstance(const guid_t& iid, IObject*& pObject) = 0;
		};

		enum Flags
		{
			Default = 0,                         ///< Use a dll/so or executable as available
			Library = 1,                         ///< Only use dll/so
			Process = 2,                         ///< Launch as current user - implies surrogate if dll/so
			Sandbox = 3,                         ///< Launch as the sandbox user - implies surrogate if dll/so

			DontLaunch = 0x10                    ///< Do not launch exe/dll/so if not already running
		};
		typedef uint16_t Flags_t;

		enum RegisterFlags
		{
			ProcessScope = 1,                   // Register for calling process only
			UserScope = (2 | ProcessScope),     // Register for calling user only
			PublicScope = (4 | UserScope),      // Register for all users
			ExternalPublic = (8 | PublicScope), // Register as externally accessible

			MultipleUse = 0,
			SingleUse = 0x10,           // Auto Revoke after 1st GetObject
			MultipleRegistration = 0x20 // Allow multiple calls to Register with different flags
		};
		typedef uint16_t RegisterFlags_t;

		interface IRunningObjectTable : public IObject
		{
			virtual uint32_t RegisterObject(const any_t& oid, IObject* pObject, Activation::RegisterFlags_t flags) = 0;
			virtual void RevokeObject(uint32_t cookie) = 0;
			virtual void GetObject(const any_t& oid, const guid_t& iid, IObject*& pObject) = 0;
		};

		interface IRunningObjectTableNotify : public IObject
		{
			virtual void OnRegisterObject(const any_t& oid, IObject* pObject, Activation::RegisterFlags_t flags) = 0;
			virtual void OnRevokeObject(const any_t& oid, IObject* pObject, Activation::RegisterFlags_t flags) = 0;
		};

		// {F67F5A41-BA32-48C9-BFD2-7B3701984DC8}
		OOCORE_DECLARE_OID(OID_RunningObjectTable);

		// {ADEA9DC0-9D82-9481-843C-CFBB8373F65E}
		OOCORE_DECLARE_OID(OID_RunningObjectTable_NoThrow);
	}

	IObject* CreateInstance(const any_t& oid, Activation::Flags_t flags, const guid_t& iid);
	IObject* GetInstance(const any_t& oid, Activation::Flags_t flags, const guid_t& iid);

	bool_t HandleRequest(uint32_t millisecs = 0);
	bool_t CanUnload();
}

#if !defined(DOXYGEN)

OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL
(
	Omega, IInternalException, Omega, IException, "{6559CBE8-DB92-4D8A-8D28-75C40E9A1F93}",

	OMEGA_METHOD(string_t,GetSource,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL
(
	Omega, ISystemException, Omega, IException, "{A0E1EEDB-BA00-4078-B67B-D990D43D5E7C}",

	OMEGA_METHOD(uint32_t,GetErrorCode,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL
(
	Omega, ITimeoutException, Omega, IException, "{63E8BFDE-D7AA-4675-B628-A1579B5AD8C7}",

	OMEGA_NO_METHODS()
)

OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL
(
	Omega, INotFoundException, Omega, IException, "{A851A685-A3AB-430b-BA52-E277655AC9CF}",

	OMEGA_NO_METHODS()
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega, IAlreadyExistsException, Omega, IException, "{5EC948EA-D7F1-4733-80A3-FF4BF5B2F4A7}",

	OMEGA_NO_METHODS()
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega, IAccessDeniedException, Omega, IException, "{A752C1AF-68CB-4fab-926A-DFC3319CEDE1}",

	OMEGA_NO_METHODS()
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega, ICastException, Omega, IException, "{F79A88F6-B2C4-490F-A11D-7D9B3894BD5D}",

	OMEGA_METHOD(any_t,GetValue,0,())
	OMEGA_METHOD(any_t::CastResult_t,GetReason,0,())
	OMEGA_METHOD(Remoting::IMessage*,GetDestinationType,0,())
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Activation, IObjectFactory, "{1BE2A9DF-A7CF-445e-8A06-C02256C4A460}",

	OMEGA_METHOD_VOID(CreateInstance,2,((in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Activation, IRunningObjectTable, "{0A36F849-8DBC-49C6-9ECA-8AD71BF3C8D0}",

	// Methods
	OMEGA_METHOD(uint32_t,RegisterObject,3,((in),const any_t&,oid,(in),IObject*,pObject,(in),Activation::RegisterFlags_t,flags))
	OMEGA_METHOD_VOID(RevokeObject,1,((in),uint32_t,cookie))
	OMEGA_METHOD_VOID(GetObject,3,((in),const any_t&,oid,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Activation, IRunningObjectTableNotify, "{006A089C-977A-5E1E-1805-BF33A5F624C7}",

	// Methods
	OMEGA_EVENT(OnRegisterObject,3,((in),const any_t&,oid,(in),IObject*,pObject,(in),Activation::RegisterFlags_t,flags))
	OMEGA_EVENT(OnRevokeObject,3,((in),const any_t&,oid,(in),IObject*,pObject,(in),Activation::RegisterFlags_t,flags))
)

#include "./internal/types.inl"
#include "./internal/any.inl"
#include "./internal/threading.inl"
#include "./internal/rtti.inl"
#include "./internal/safe.inl"
#include "./internal/wire.inl"

#if defined(_MSC_VER) && defined(_Wp64)
#pragma warning(pop)
#endif

OOCORE_EXPORTED_FUNCTION(Omega::IException*,OOCore_Omega_Initialize,1,((in),Omega::uint32_t,version))
inline Omega::IException* Omega::Initialize()
{
	try
	{
		return OOCore_Omega_Initialize((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16));
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

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_GetInstance,4,((in),const Omega::any_t&,oid,(in),Omega::Activation::Flags_t,flags,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject));
inline Omega::IObject* Omega::GetInstance(const any_t& oid, Activation::Flags_t flags, const guid_t& iid)
{
	IObject* pObject = NULL;
	OOCore_GetInstance(oid,flags,iid,pObject);
	return pObject;
}

inline Omega::IObject* Omega::CreateInstance(const any_t& oid, Activation::Flags_t flags, const guid_t& iid)
{
	IObject* pObject = NULL;
	OOCore_GetInstance(oid,flags,OMEGA_GUIDOF(Activation::IObjectFactory),pObject);
	System::Internal::auto_iface_ptr<Activation::IObjectFactory> ptrOF = static_cast<Activation::IObjectFactory*>(pObject);

	pObject = NULL;
	ptrOF->CreateInstance(iid,pObject);
	return pObject;
}

OOCORE_EXPORTED_FUNCTION(Omega::bool_t,OOCore_Omega_HandleRequest,1,((in),Omega::uint32_t,millisecs));
inline Omega::bool_t Omega::HandleRequest(uint32_t millisecs)
{
	// millisecs == 0 is INFINITE
	return OOCore_Omega_HandleRequest(millisecs);
}

OOCORE_EXPORTED_FUNCTION(Omega::bool_t,OOCore_Omega_CanUnload,0,());
inline Omega::bool_t Omega::CanUnload()
{
	return OOCore_Omega_CanUnload();
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_allocate,1,((in),size_t,bytes));
inline void* Omega::System::Allocate(size_t bytes)
{
	return OOCore_allocate(bytes);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_free,1,((in),void*,mem));
inline void Omega::System::Free(void* mem)
{
	OOCore_free(mem);
}

#endif // !defined(DOXYGEN)

#endif // OMEGA_H_INCLUDED_
