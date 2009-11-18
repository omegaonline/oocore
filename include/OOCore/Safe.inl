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

#ifndef OOCORE_SAFE_INL_INCLUDED_
#define OOCORE_SAFE_INL_INCLUDED_

void Omega::System::MetaInfo::safe_proxy_holder::remove(const SafeShim* shim)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);
		m_map.erase(shim);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Safe_Proxy_Owner> Omega::System::MetaInfo::safe_proxy_holder::find(const SafeShim* shim)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::map<const SafeShim*,Safe_Proxy_Owner*>::const_iterator i=m_map.find(shim);
		if (i != m_map.end())
		{
			i->second->AddRef();
			return auto_iface_ptr<Safe_Proxy_Owner>(i->second);
		}

		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Safe_Proxy_Owner> Omega::System::MetaInfo::safe_proxy_holder::add(const SafeShim* shim, Safe_Proxy_Owner* pOwner)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::pair<std::map<const SafeShim*,Safe_Proxy_Owner*>::iterator,bool> p = m_map.insert(std::map<const SafeShim*,Safe_Proxy_Owner*>::value_type(shim,pOwner));
		if (!p.second)
		{
			p.first->second->AddRef();
			return auto_iface_ptr<Safe_Proxy_Owner>(p.first->second);
		}

		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void Omega::System::MetaInfo::safe_stub_holder::remove(IObject* pObject)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);
		m_map.erase(pObject);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Safe_Stub_Owner> Omega::System::MetaInfo::safe_stub_holder::find(IObject* pObject)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::map<IObject*,Safe_Stub_Owner*>::const_iterator i=m_map.find(pObject);
		if (i != m_map.end())
		{
			i->second->AddRef();
			return auto_iface_ptr<Safe_Stub_Owner>(i->second);
		}

		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Safe_Stub_Owner> Omega::System::MetaInfo::safe_stub_holder::add(IObject* pObject, Safe_Stub_Owner* pOwner)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::pair<std::map<IObject*,Safe_Stub_Owner*>::iterator,bool> p = m_map.insert(std::map<IObject*,Safe_Stub_Owner*>::value_type(pObject,pOwner));
		if (!p.second)
		{
			p.first->second->AddRef();
			return auto_iface_ptr<Safe_Stub_Owner>(p.first->second);
		}

		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Omega::System::MetaInfo::Safe_Proxy_Owner::~Safe_Proxy_Owner()
{
	SAFE_PROXY_HOLDER::instance()->remove(m_base_shim);

	const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_base_shim->m_vtable)->pfnUnpin_Safe(m_base_shim);
	if (except)
		static_cast<const IObject_Safe_VTable*>(except->m_vtable)->pfnRelease_Safe(except);
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Safe_Proxy_Base> Omega::System::MetaInfo::Safe_Proxy_Owner::GetProxyBase(const guid_t& iid, const SafeShim* shim)
{
	assert(iid != OMEGA_GUIDOF(IObject));
	assert(iid != OMEGA_GUIDOF(ISafeProxy));

	Threading::Guard<Threading::Mutex> guard(m_lock);

	try
	{
		// See if we have it cached
		std::map<guid_t,Safe_Proxy_Base*>::iterator i=m_iid_map.find(iid);
		if (i == m_iid_map.end())
		{
			// See if we have a derived iid
			for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
			{
				if (i->second->IsDerived__proxy__(iid))
				{
					m_iid_map.insert(std::map<guid_t,Safe_Proxy_Base*>::value_type(iid,i->second));
					break;
				}
			}
		}

		if (i != m_iid_map.end())
		{
			i->second->AddRef();
			return auto_iface_ptr<Safe_Proxy_Base>(i->second);
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	// QueryInterface the actual interface pointer
	auto_safe_shim ss;
	if (!shim)
	{
		const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_base_shim->m_vtable)->pfnQueryInterface_Safe(m_base_shim,&shim,&iid);
		if (except)
			throw_correct_exception(except);

		if (!shim)
			return 0;

		// Ensure its released
		ss.attach(shim);
	}

	// Wrap it in a proxy and add it...
	const qi_rtti* rtti = get_qi_rtti_info(*shim->m_iid);
	if (!rtti && guid_t(*shim->m_iid) != iid)
		rtti = get_qi_rtti_info(iid);

	if (!rtti)
		OMEGA_THROW(L"Failed to create proxy for interface - missing rtti");

	auto_iface_ptr<Safe_Proxy_Base> obj = (*rtti->pfnCreateSafeProxy)(shim,this);
	if (!obj)
		OMEGA_THROW(L"Failed to create safe proxy");

	try
	{
		if (iid != guid_t(*shim->m_iid))
			m_iid_map.insert(std::map<guid_t,Safe_Proxy_Base*>::value_type(*shim->m_iid,obj));

		m_iid_map.insert(std::map<guid_t,Safe_Proxy_Base*>::value_type(iid,obj));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	return obj;
}

void Omega::System::MetaInfo::Safe_Proxy_Owner::RemoveBase(Safe_Proxy_Base* pProxy)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		for (std::map<guid_t,Safe_Proxy_Base*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();)
		{
			if (i->second == pProxy)
				m_iid_map.erase(i++);
			else
				++i;
		}

		if (m_iid_map.empty() && m_refcount.IsZero() && m_pincount.IsZero())
		{
			guard.Release();
			delete this;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Safe_Proxy_Owner::GetShim(const guid_t& iid)
{
	if (iid == OMEGA_GUIDOF(IObject))
	{
		const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_base_shim->m_vtable)->pfnAddRef_Safe(m_base_shim);
		if (except)
			throw_correct_exception(except);

		return m_base_shim;
	}

	// See if we have it cached
	auto_iface_ptr<Safe_Proxy_Base> obj = GetProxyBase(iid,0);
	if (!obj)
		OMEGA_THROW(L"Failed to create safe proxy");

	// Return the shim
	return obj->GetShim();
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Safe_Proxy_Owner::CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid)
{
	assert(iid != OMEGA_GUIDOF(IObject));

	if (!static_cast<const IObject_Safe_VTable*>(m_base_shim->m_vtable)->pfnCreateWireStub_Safe)
		return 0;

	const SafeShim* ret = 0;
	const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_base_shim->m_vtable)->pfnCreateWireStub_Safe(m_base_shim,shim_Controller,shim_Marshaller,&iid,&ret);
	if (except)
		throw_correct_exception(except);

	return ret;
}

Omega::IObject* Omega::System::MetaInfo::Safe_Proxy_Owner::QueryInterface(const guid_t& iid)
{
	if (iid == OMEGA_GUIDOF(IObject))
	{
		if (m_pOuter)
		{
			m_pOuter->AddRef();
			return m_pOuter;
		}
		else
		{
			m_internal.AddRef();
			return &m_internal;
		}
	}
	else if (iid == OMEGA_GUIDOF(ISafeProxy))
	{
		m_safe_proxy.AddRef();
		return &m_safe_proxy;
	}

	// See if we have it cached
	auto_iface_ptr<Safe_Proxy_Base> obj = GetProxyBase(iid,0);
	if (!obj)
		return 0;

	// Return cast to the correct type
	return obj->QIReturn__proxy__();
}

Omega::IObject* Omega::System::MetaInfo::Safe_Proxy_Owner::CreateProxy(const SafeShim* shim)
{
	assert(guid_t(*shim->m_iid) != OMEGA_GUIDOF(ISafeProxy));

	if (guid_t(*shim->m_iid) == OMEGA_GUIDOF(IObject))
	{
		m_internal.AddRef();
		return &m_internal;
	}

	// See if we have it cached
	auto_iface_ptr<Safe_Proxy_Base> obj = GetProxyBase(*shim->m_iid,shim);
	if (!obj)
		return 0;

	// Return cast to the correct type
	return obj->QIReturn__proxy__();
}

void Omega::System::MetaInfo::Safe_Proxy_Owner::Throw(const SafeShim* shim)
{
	assert(guid_t(*shim->m_iid) != OMEGA_GUIDOF(ISafeProxy));
	assert(guid_t(*shim->m_iid) != OMEGA_GUIDOF(IObject));

	// See if we have it cached
	auto_iface_ptr<Safe_Proxy_Base> obj = GetProxyBase(*shim->m_iid,shim);
	if (!obj)
		OMEGA_THROW(L"Failed to throw safe proxy");
	
	// Release the incoming shim....
	const SafeShim* except = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnRelease_Safe(shim);
	if (except)
		throw_correct_exception(except);

	return obj->Throw__proxy__();
}

void Omega::System::MetaInfo::Safe_Proxy_Owner::Throw(const guid_t& iid)
{
	assert(iid != OMEGA_GUIDOF(ISafeProxy));
	assert(iid != OMEGA_GUIDOF(IObject));

	// See if we have it cached
	auto_iface_ptr<Safe_Proxy_Base> obj = GetProxyBase(iid,0);
	if (!obj)
		OMEGA_THROW(L"Failed to throw safe proxy");
	
	return obj->Throw__proxy__();
}

Omega::System::MetaInfo::Safe_Proxy_Base::~Safe_Proxy_Base()
{
	m_pOwner->RemoveBase(this);
}

void Omega::System::MetaInfo::Safe_Proxy_Base::Throw(const guid_t& iid)
{
	m_pOwner->Throw(iid);
}

Omega::IObject* Omega::System::MetaInfo::Safe_Proxy_Base::QueryInterface(const guid_t& iid)
{
	if (iid == OMEGA_GUIDOF(ISafeProxy))
	{
		Internal_AddRef();
		return &m_internal;
	}

	return m_pOwner->QueryInterface(iid);
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Safe_Proxy_Base::GetShim(const guid_t& iid)
{
	if (IsDerived__proxy__(iid))
		return GetShim();

	return m_pOwner->GetShim(iid);
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Safe_Proxy_Base::CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid)
{
	return m_pOwner->CreateWireStub(shim_Controller,shim_Marshaller,iid);
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Safe_Proxy_Owner> Omega::System::MetaInfo::create_safe_proxy_owner(const SafeShim* shim, IObject* pOuter)
{
	// QI for the IObject shim
	const SafeShim* base_shim;
	const SafeShim* except = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnGetBaseShim_Safe(shim,&base_shim);
	if (except)
		throw_correct_exception(except);

	auto_safe_shim ss_base = base_shim;

	// Lookup in the global map...
	auto_iface_ptr<Safe_Proxy_Owner> ptrOwner = SAFE_PROXY_HOLDER::instance()->find(base_shim);
	if (ptrOwner)
		return ptrOwner;

	// Create a safe proxy owner
	OMEGA_NEW(ptrOwner,Safe_Proxy_Owner(base_shim,pOuter));

	// Add to the map...
	auto_iface_ptr<Safe_Proxy_Owner> ptrExisting = SAFE_PROXY_HOLDER::instance()->add(base_shim,ptrOwner);
	if (ptrExisting)
		return ptrExisting;

	return ptrOwner;
}

Omega::IObject* Omega::System::MetaInfo::create_safe_proxy(const SafeShim* shim, IObject* pOuter)
{
	if (!shim)
		return 0;

	// See if we are a Wire Proxy
	if (static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnGetWireProxy_Safe)
	{
		const SafeShim* proxy = 0;
		const SafeShim* pE = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnGetWireProxy_Safe(shim,&proxy);
		if (pE)
			throw_correct_exception(pE);

		auto_safe_shim ss_proxy = proxy;

		auto_iface_ptr<Wire_Proxy_Owner> ptrOwner = create_wire_proxy_owner(ss_proxy,pOuter);

		IObject* pRet = ptrOwner->CreateProxy(*shim->m_iid);
		if (!pRet)
			OMEGA_THROW(L"Failed to find correct shim for wire_iid");

		return pRet;
	}

	return create_safe_proxy_owner(shim,pOuter)->CreateProxy(shim);
}

void Omega::System::MetaInfo::throw_correct_exception(const SafeShim* shim)
{
	create_safe_proxy_owner(shim,0)->Throw(shim);
}

Omega::System::MetaInfo::Safe_Stub_Owner::~Safe_Stub_Owner()
{
	SAFE_STUB_HOLDER::instance()->remove(m_pI);
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Safe_Stub_Base> Omega::System::MetaInfo::Safe_Stub_Owner::GetStubBase(const guid_t& iid, IObject* pObj)
{
	assert(iid != OMEGA_GUIDOF(IObject));

	Threading::Guard<Threading::Mutex> guard(m_lock);

	// See if we have it cached
	try
	{
		std::map<guid_t,Safe_Stub_Base*>::iterator i=m_iid_map.find(iid);
		if (i == m_iid_map.end())
		{
			// See if we have a derived iid
			for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
			{
				if (i->second->IsDerived(iid))
				{
					m_iid_map.insert(std::map<guid_t,Safe_Stub_Base*>::value_type(iid,i->second));
					break;
				}
			}
		}

		if (i != m_iid_map.end())
		{
			i->second->AddRef();
			return auto_iface_ptr<Safe_Stub_Base>(i->second);
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	// Check the actual interface pointer
	auto_iface_ptr<IObject> iface;

	if (!pObj)
	{
		pObj = m_pI->QueryInterface(iid);
		if (!pObj)
			return 0;

		// Ensure its released
		iface.attach(pObj);
	}

	// Wrap it in a shim and add it...
	const qi_rtti* rtti = get_qi_rtti_info(iid);
	if (!rtti)
		OMEGA_THROW(L"Failed to create stub for interface - missing rtti");

	auto_iface_ptr<Safe_Stub_Base> pStub = (*rtti->pfnCreateSafeStub)(pObj,this);
	if (!pStub)
		OMEGA_THROW(L"Failed to create safe stub");

	try
	{
		m_iid_map.insert(std::map<guid_t,Safe_Stub_Base*>::value_type(iid,pStub));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	return pStub;
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Safe_Stub_Owner::QueryInterface(const guid_t& iid, IObject* pObj)
{
	// We can safely return ourselves for IObject
	if (iid == OMEGA_GUIDOF(IObject))
	{
		AddRef();
		return &m_base_shim;
	}

	// See if we have it cached
	auto_iface_ptr<Safe_Stub_Base> pStub = GetStubBase(iid,pObj);
	if (!pStub)
		return 0;

	return pStub->GetShim();
}

void Omega::System::MetaInfo::Safe_Stub_Owner::RemoveBase(Safe_Stub_Base* pStub)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		for (std::map<guid_t,Safe_Stub_Base*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();)
		{
			if (i->second == pStub)
				m_iid_map.erase(i++);
			else
				++i;
		}

		if (m_iid_map.empty() && m_refcount.IsZero() && m_pincount.IsZero())
		{
			guard.Release();
			delete this;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Safe_Stub_Owner::CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid)
{
	return create_wire_stub(shim_Controller,shim_Marshaller,iid,m_pI);
}

Omega::System::MetaInfo::Safe_Stub_Base::~Safe_Stub_Base()
{
	m_pOwner->RemoveBase(this);
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Safe_Stub_Base::QueryInterface(const guid_t& iid)
{
	if (iid != OMEGA_GUIDOF(IObject) && IsDerived(iid))
	{
		AddRef();
		return &m_shim;
	}

	return m_pOwner->QueryInterface(iid,0);
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Safe_Stub_Base::GetBaseShim()
{
	return m_pOwner->GetBaseShim();
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Safe_Stub_Owner> Omega::System::MetaInfo::create_safe_stub_owner(IObject* pObj)
{
	// Get the IObject interface
	auto_iface_ptr<IObject> ptrObject = static_cast<IObject*>(pObj->QueryInterface(OMEGA_GUIDOF(IObject)));

	// Lookup in the global map...
	auto_iface_ptr<Safe_Stub_Owner> ptrOwner = SAFE_STUB_HOLDER::instance()->find(ptrObject);
	if (ptrOwner)
		return ptrOwner;

	// Create a safe proxy owner
	OMEGA_NEW(ptrOwner,Safe_Stub_Owner(ptrObject));

	// Add to the map...
	auto_iface_ptr<Safe_Stub_Owner> ptrExisting = SAFE_STUB_HOLDER::instance()->add(ptrObject,ptrOwner);
	if (ptrExisting)
		return ptrExisting;

	return ptrOwner;
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::create_safe_stub(IObject* pObj, const guid_t& iid)
{
	if (!pObj)
		return 0;

	// See if pObj is actually a proxy...
	auto_iface_ptr<ISafeProxy> ptrProxy = static_cast<ISafeProxy*>(pObj->QueryInterface(OMEGA_GUIDOF(ISafeProxy)));
	if (ptrProxy)
		return ptrProxy->GetShim(iid);

	// QI and return
	return create_safe_stub_owner(pObj)->QueryInterface(iid,pObj);
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::return_safe_exception(IException* pE)
{
	auto_iface_ptr<IException> ptrE(pE);
	return create_safe_stub(pE,pE->GetThrownIID());
}

#endif // OOCORE_SAFE_INL_INCLUDED_
