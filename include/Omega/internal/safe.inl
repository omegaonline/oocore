///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
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

#ifndef OMEGA_SAFE_INL_INCLUDED_
#define OMEGA_SAFE_INL_INCLUDED_

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rtti_holder__ctor,2,((in),void**,phandle,(in),Omega::Threading::SingletonCallback,pfn_init));
inline void* Omega::System::Internal::rtti_holder::handle()
{
	static void* s_handle = NULL;
	OOCore_rtti_holder__ctor(&s_handle,&init);
	return s_handle;
}

inline const Omega::System::Internal::SafeShim* OMEGA_CALL Omega::System::Internal::rtti_holder::init(void** param)
{
	const SafeShim* except = NULL;
	try
	{
		Threading::ModuleDestructor<OMEGA_PRIVATE_TYPE(safe_module)>::add_destructor(destroy,*param);
	}
	catch (IException* pE)
	{
		except = return_safe_exception(pE);
	}
	return except;
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rtti_holder__dctor,1,((in),void*,handle));
inline void Omega::System::Internal::rtti_holder::destroy(void* param)
{
	OOCore_rtti_holder__dctor(param);
}

OOCORE_RAW_EXPORTED_FUNCTION(const Omega::System::Internal::qi_rtti*,OOCore_rtti_holder_find_qi,2,((in),void*,handle,(in),const Omega::guid_base_t*,iid));
inline const Omega::System::Internal::qi_rtti* Omega::System::Internal::get_qi_rtti_info(const Omega::guid_t& iid)
{
	return OOCore_rtti_holder_find_qi(rtti_holder::handle(),&iid);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rtti_holder_insert_qi,3,((in),void*,handle,(in),const Omega::guid_base_t*,iid,(in),const Omega::System::Internal::qi_rtti*,pRtti));
inline void Omega::System::Internal::register_qi_rtti_info(const Omega::guid_t& iid, const Omega::System::Internal::qi_rtti* pRtti)
{
	OOCore_rtti_holder_insert_qi(rtti_holder::handle(),&iid,pRtti);
}

OOCORE_RAW_EXPORTED_FUNCTION(const Omega::System::Internal::wire_rtti*,OOCore_rtti_holder_find_wi,2,((in),void*,handle,(in),const Omega::guid_base_t*,iid));
inline const Omega::System::Internal::wire_rtti* Omega::System::Internal::get_wire_rtti_info(const guid_t& iid)
{
	return OOCore_rtti_holder_find_wi(rtti_holder::handle(),&iid);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rtti_holder_insert_wi,3,((in),void*,handle,(in),const Omega::guid_base_t*,iid,(in),const Omega::System::Internal::wire_rtti*,pRtti));
inline void Omega::System::Internal::register_wire_rtti_info(const guid_t& iid, const wire_rtti* pRtti)
{
	OOCore_rtti_holder_insert_wi(rtti_holder::handle(),&iid,pRtti);
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_safe_holder__ctor,0,());
inline Omega::System::Internal::safe_holder::safe_holder() : m_handle(NULL)
{
	m_handle = OOCore_safe_holder__ctor();
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_safe_holder__dctor,1,((in),void*,handle));
inline Omega::System::Internal::safe_holder::~safe_holder()
{
	OOCore_safe_holder__dctor(m_handle);
}

OOCORE_RAW_EXPORTED_FUNCTION(Omega::IObject*,OOCore_safe_holder_add1,3,((in),void*,handle,(in),const Omega::System::Internal::SafeShim*,shim,(in),Omega::IObject*,pObject));
inline Omega::IObject* Omega::System::Internal::safe_holder::add(const SafeShim* shim, IObject* pObject)
{
	IObject* pRet = OOCore_safe_holder_add1(m_handle,shim,pObject);
	if (pRet)
		pRet->AddRef();

	return pRet;
}

OOCORE_RAW_EXPORTED_FUNCTION(const Omega::System::Internal::SafeShim*,OOCore_safe_holder_add2,3,((in),void*,handle,(in),Omega::IObject*,pObject,(in),const Omega::System::Internal::SafeShim*,shim));
inline const Omega::System::Internal::SafeShim* Omega::System::Internal::safe_holder::add(IObject* pObject, const SafeShim* shim)
{
	const SafeShim* pRet = OOCore_safe_holder_add2(m_handle,pObject,shim);
	if (pRet)
		addref_safe(pRet);

	return pRet;
}

OOCORE_RAW_EXPORTED_FUNCTION(const Omega::System::Internal::SafeShim*,OOCore_safe_holder_find,2,((in),void*,handle,(in),Omega::IObject*,pObject));
inline const Omega::System::Internal::SafeShim* Omega::System::Internal::safe_holder::find(IObject* pObject)
{
	const SafeShim* pRet = OOCore_safe_holder_find(m_handle,pObject);
	if (pRet)
		addref_safe(pRet);

	return pRet;
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_safe_holder_remove1,2,((in),void*,handle,(in),Omega::IObject*,pObject))
inline void Omega::System::Internal::safe_holder::remove(IObject* pObject)
{
	OOCore_safe_holder_remove1(m_handle,pObject);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_safe_holder_remove2,2,((in),void*,handle,(in),const Omega::System::Internal::SafeShim*,shim))
inline void Omega::System::Internal::safe_holder::remove(const SafeShim* shim)
{
	OOCore_safe_holder_remove2(m_handle,shim);
}

inline Omega::IObject* Omega::System::Internal::Safe_Proxy_Base::QueryInterface(const guid_t& iid)
{
	if (iid == OMEGA_GUIDOF(ISafeProxy))
	{
		m_internal.AddRef();
		return &m_internal;
	}
	else if (IsDerived__proxy__(iid))
	{
		AddRef();
		return QIReturn__proxy__();
	}

	// QI m_shim
	auto_safe_shim retval;
	const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnQueryInterface_Safe(m_shim,&retval,&iid);
	throw_correct_exception(except);

	return create_safe_proxy(retval,iid);
}

inline Omega::IObject* Omega::System::Internal::create_safe_proxy(const SafeShim* shim, const guid_t& iid)
{
	if (!shim)
		return NULL;
	
	// See if we are a Wire Proxy
	if (static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnGetWireProxy_Safe)
	{
		// Retrieve the underlying proxy
		auto_safe_shim proxy;
		const SafeShim* pE = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnGetWireProxy_Safe(shim,&proxy);
		throw_correct_exception(pE);

		// Control its lifetime
		auto_iface_ptr<Remoting::IProxy> ptrProxy = create_safe_proxy<Remoting::IProxy>(proxy);

		// Create a wire proxy
		return create_wire_proxy(ptrProxy,iid);
	}
	
	// Check for special case of IObject
	if (guid_t(*shim->m_iid) == OMEGA_GUIDOF(IObject))
		return Safe_Proxy_IObject::bind(shim);
		
	// Find the rtti info...
	const qi_rtti* rtti = get_qi_rtti_info(*shim->m_iid);
	if (!rtti && guid_t(*shim->m_iid) != iid)
		rtti = get_qi_rtti_info(iid);
	
	// Fall back to IObject for completely unknown interfaces
	if (!rtti)
		rtti = get_qi_rtti_info(OMEGA_GUIDOF(IObject));

	return (*rtti->pfnCreateSafeProxy)(shim);
}

inline void Omega::System::Internal::throw_correct_exception(const SafeShim* shim)
{
	if (shim)
	{
		// Ensure shim is released
		auto_safe_shim ss = shim;

		create_safe_proxy<IException>(shim)->Rethrow();
	}
}

inline const Omega::System::Internal::SafeShim* Omega::System::Internal::create_safe_stub(IObject* pObj, const guid_t& iid)
{
	if (!pObj)
		return NULL;
	
	// See if we have it cached...
	if (iid == OMEGA_GUIDOF(IObject))
	{
		const SafeShim* shim = SAFE_HOLDER::instance()->find(pObj);
		if (shim)
			return shim;
	}

	// See if pObj is actually a proxy...
	auto_iface_ptr<ISafeProxy> ptrProxy = static_cast<ISafeProxy*>(pObj->QueryInterface(OMEGA_GUIDOF(ISafeProxy)));
	if (ptrProxy)
	{
		const SafeShim* shim = ptrProxy->GetShim(iid);
		if (shim)
			return shim;
	}
	
	// Check for special case of IObject
	if (iid == OMEGA_GUIDOF(IObject))
		return Safe_Stub_IObject::create(pObj);
	
	// Find the rtti info...
	const qi_rtti* rtti = get_qi_rtti_info(iid);
	if (!rtti)
		throw OOCore_INotFoundException_MissingRTTI(iid);
		
	return (*rtti->pfnCreateSafeStub)(pObj);
}

inline const Omega::System::Internal::SafeShim* Omega::System::Internal::return_safe_exception(IException* pE)
{
	auto_iface_ptr<IException> ptrE(pE);
	return create_safe_stub(pE,pE->GetThrownIID());
}

#endif // OMEGA_SAFE_INL_INCLUDED_
