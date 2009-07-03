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

void Omega::System::MetaInfo::proxy_holder::remove(const SafeShim* shim)
{
	Threading::Guard<Threading::Mutex> guard(m_lock);
	m_map.erase(shim);
}

Omega::System::MetaInfo::Safe_Proxy_Owner* Omega::System::MetaInfo::proxy_holder::find(const SafeShim* shim)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::map<const SafeShim*,Safe_Proxy_Owner*>::const_iterator i=m_map.find(shim);
		if (i != m_map.end())
		{
			Safe_Proxy_Owner* pRet = i->second;
			pRet->Internal_AddRef();
			return pRet;
		}
		
		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Omega::System::MetaInfo::Safe_Proxy_Owner* Omega::System::MetaInfo::proxy_holder::add(const SafeShim* shim, Safe_Proxy_Owner* pOwner)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::pair<std::map<const SafeShim*,Safe_Proxy_Owner*>::iterator,bool> p = m_map.insert(std::map<const SafeShim*,Safe_Proxy_Owner*>::value_type(shim,pOwner));
		if (!p.second)
		{
			Safe_Proxy_Owner* pRet = p.first->second;
			pRet->Internal_AddRef();
			return pRet;
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
	try
	{
		PROXY_HOLDER::instance()->remove(m_shim);

		for (std::map<guid_t,Safe_Proxy_Base*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
			i->second->DecRef();
	}
	catch (...)
	{}
}

Omega::System::MetaInfo::Safe_Proxy_Base* Omega::System::MetaInfo::Safe_Proxy_Owner::GetBase(const Omega::guid_t& iid, const SafeShim* shim)
{
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
				if (i->second->IsDerived(iid))
				{
					m_iid_map.insert(std::map<guid_t,Safe_Proxy_Base*>::value_type(iid,i->second));
					i->second->IncRef();
					break;
				}
			}
		}

		if (i != m_iid_map.end())
		{
			return i->second;
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
		const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnQueryInterface_Safe(m_shim,&shim,&iid);
		if (except)
			throw_correct_exception(except);

		if (!shim)
			return 0;

		// Ensure its released
		ss.attach(shim);
	}

	// Wrap it in a proxy and add it...
	const qi_rtti* rtti = get_qi_rtti_info(*shim->m_iid);
	if (!rtti)
		rtti = get_qi_rtti_info(iid);
		
	if (!rtti)
		rtti = get_qi_rtti_info(OMEGA_GUIDOF(IObject));

	if (!rtti)
		OMEGA_THROW(L"Failed to create proxy for interface - missing rtti");

	Safe_Proxy_Base* obj = (*rtti->pfnCreateSafeProxy)(shim,this);
	if (!obj)
		OMEGA_THROW(L"Failed to create safe proxy");

	try
	{
		if (iid != *shim->m_iid)
		{
			obj->IncRef();
			m_iid_map.insert(std::map<guid_t,Safe_Proxy_Base*>::value_type(*shim->m_iid,obj));
		}

		m_iid_map.insert(std::map<guid_t,Safe_Proxy_Base*>::value_type(iid,obj));
	}
	catch (std::exception& e)
	{
		obj->DecRef();
		OMEGA_THROW(e);
	}

	return obj;
}

Omega::IObject* Omega::System::MetaInfo::Safe_Proxy_Owner::QueryInterface(const Omega::guid_t& iid, const SafeShim* shim)
{
	// Always return the same object for IObject
	if (iid == OMEGA_GUIDOF(IObject))
	{
		Internal_AddRef();
		if (m_pOuter)
			return &m_agg_object;
		else
			return static_cast<IObject*>(this);
	}
	else if (iid == OMEGA_GUIDOF(ISafeProxy))
	{
		Private_AddRef();
		return &m_internal_safe_proxy;
	}	

	// See if we have it cached
	Safe_Proxy_Base* obj = GetBase(iid,shim);
	if (!obj)
		return 0;

	Internal_AddRef();
	return obj->QIReturn();
}

void Omega::System::MetaInfo::Safe_Proxy_Owner::Throw(const Omega::guid_t& iid, const SafeShim* shim)
{
	// See if we have it cached
	Safe_Proxy_Base* obj = GetBase(iid,shim);
	if (!obj)
		OMEGA_THROW(L"Failed to create safe proxy");

	// Release the incoming shim....
	const SafeShim* except = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnRelease_Safe(shim);
	if (except)
		throw_correct_exception(except);

	if (!m_pOuter)
		Internal_AddRef();
	
	return obj->Throw();
}

Omega::System::MetaInfo::Safe_Proxy_Owner* Omega::System::MetaInfo::create_proxy_owner(const SafeShim* shim, IObject* pOuter)
{
	// QI for the IObject shim
	const SafeShim* base_shim;
	const SafeShim* except = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnGetBaseShim_Safe(shim,&base_shim);
	if (except)
		throw_correct_exception(except);

	// Lookup in the global map...
	Safe_Proxy_Owner* pOwner = PROXY_HOLDER::instance()->find(base_shim);
	if (pOwner)
		return pOwner;
	else
	{
		// Create a safe proxy owner
		OMEGA_NEW(pOwner,Safe_Proxy_Owner(base_shim,pOuter));
		
		// Add to the map...
		try
		{
			Safe_Proxy_Owner* pExisting = PROXY_HOLDER::instance()->add(base_shim,pOwner);
			if (pExisting)
			{
				pOwner->Internal_Release();
				return pExisting;
			}
		}
		catch (...)
		{
			pOwner->Release();
			throw;
		}

		return pOwner;
	}
}

Omega::IObject* Omega::System::MetaInfo::create_proxy(const SafeShim* shim, IObject* pOuter)
{
	if (!shim)
		return 0;

	// QI and return
	Safe_Proxy_Owner* pOwner = create_proxy_owner(shim,pOuter);
	try
	{
		IObject* pRet = pOwner->QueryInterface(*shim->m_iid,shim);
		pOwner->Internal_Release();
		return pRet;
	}
	catch (...)
	{
		pOwner->Internal_Release();
		throw;
	}
}

void Omega::System::MetaInfo::throw_correct_exception(const SafeShim* shim)
{
	create_proxy_owner(shim,0)->Throw(*shim->m_iid,shim);
}

OMEGA_EXPORTED_FUNCTION(void*,OOCore_safe_stub_find,1,((in),void*,pObject));
OMEGA_EXPORTED_FUNCTION(void*,OOCore_safe_stub_add,2,((in),void*,pObject,(in),void*,pStub));
OMEGA_EXPORTED_FUNCTION_VOID(OOCore_safe_stub_remove,1,((in),void*,pObject));

Omega::System::MetaInfo::Safe_Stub_Owner::~Safe_Stub_Owner()
{
	try
	{
		OOCore_safe_stub_remove(m_pObject);

		for (std::map<guid_t,Safe_Stub_Base*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
			i->second->DecRef();
	}
	catch (...)
	{}
}

Omega::System::MetaInfo::Safe_Stub_Base* Omega::System::MetaInfo::Safe_Stub_Owner::GetBase(const guid_t& iid, IObject* pObj)
{
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
					i->second->IncRef();
					break;
				}
			}
		}

		if (i != m_iid_map.end())
		{
			return i->second;
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
		pObj = m_pObject->QueryInterface(iid);
		if (!pObj)
			return 0;

		// Ensure its released
		iface.attach(pObj);
	}

	// Wrap it in a shim and add it...
	const qi_rtti* rtti = get_qi_rtti_info(iid);
	if (!rtti)
		OMEGA_THROW(L"Failed to create stub for interface - missing rtti");
	
	Safe_Stub_Base* pStub = (*rtti->pfnCreateSafeStub)(pObj,this);
	if (!pStub)
		OMEGA_THROW(L"Failed to create safe stub");
	
	try
	{
		m_iid_map.insert(std::map<guid_t,Safe_Stub_Base*>::value_type(iid,pStub));
	}
	catch (std::exception& e)
	{
		pStub->DecRef();
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
		return &m_shim;
	}

	// See if we have it cached
	Safe_Stub_Base* pStub = GetBase(iid,pObj);
	if (!pStub)
		return 0;

	AddRef();
	return pStub->GetShim();
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Safe_Stub_Owner> Omega::System::MetaInfo::create_stub_owner(IObject* pObj)
{
	// Get the IObject interface
	auto_iface_ptr<IObject> ptrObject = static_cast<IObject*>(pObj->QueryInterface(OMEGA_GUIDOF(IObject)));

	auto_iface_ptr<Safe_Stub_Owner> ptrOwner;

	// Lookup in the global map...
	ptrOwner = static_cast<Safe_Stub_Owner*>(OOCore_safe_stub_find(static_cast<IObject*>(ptrObject)));
	if (ptrOwner)
		ptrOwner->AddRef();
	else
	{
		// Create a safe proxy owner
		OMEGA_NEW(ptrOwner,Safe_Stub_Owner(ptrObject));
		
		// Add to the map...
		Safe_Stub_Owner* pExisting = static_cast<Safe_Stub_Owner*>(OOCore_safe_stub_add(static_cast<IObject*>(ptrObject),static_cast<Safe_Stub_Owner*>(ptrOwner)));
		if (pExisting)
		{
			ptrOwner = pExisting;
			ptrOwner->AddRef();
		}
	}
	
	return ptrOwner;
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::create_stub(IObject* pObj, const guid_t& iid)
{
	if (!pObj)
		return 0;

	// See if pObj is actually a proxy...
	auto_iface_ptr<ISafeProxy> ptrProxy = static_cast<ISafeProxy*>(pObj->QueryInterface(OMEGA_GUIDOF(ISafeProxy)));
	if (ptrProxy)
		return ptrProxy->GetStub(iid);

	// QI and return
	return create_stub_owner(pObj)->QueryInterface(iid,pObj);
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::return_safe_exception(IException* pE)
{
	auto_iface_ptr<IException> ptrE(pE);
	return create_stub(pE,ptrE->GetThrownIID());
}

#endif // OOCORE_SAFE_INL_INCLUDED_
