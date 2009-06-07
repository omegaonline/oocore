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

Omega::System::MetaInfo::Safe_Proxy_Base* Omega::System::MetaInfo::Safe_Proxy_Owner::GetBase(const Omega::guid_t& iid)
{
	Threading::Guard<Threading::Mutex> guard(m_lock);

	// See if we have it cached
	std::map<guid_t,Safe_Proxy_Base*>::iterator i=m_iid_map.find(iid);
	if (i == m_iid_map.end())
	{
		// See if we have a derived iid
		for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
		{
			if (static_cast<Safe_Proxy_Base*>(i->second)->IsDerived(iid))
			{
				m_iid_map[iid] = i->second;
				i->second->IncRef();
				break;
			}
		}
	}

	if (i != m_iid_map.end())
	{
		return i->second;
	}

	// QueryInterface the actual interface pointer
	SafeShim* shim = 0;
	SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnQueryInterface_Safe(m_shim,&shim,&iid);
	if (except)
		throw_correct_exception(except);

	if (!shim)
		return 0;

	// Wrap it in a proxy and add it...
	const qi_rtti* rtti = get_qi_rtti_info(*shim->m_iid);
	if (!rtti)
		rtti = get_qi_rtti_info(iid);
		
	if (!rtti)
		OMEGA_THROW(L"Failed to create proxy for interface - missing rtti");

	Safe_Proxy_Base* obj = (*rtti->pfnCreateSafeProxy)(shim,this);
	if (!obj)
		OMEGA_THROW(L"Failed to create safe proxy");

	if (iid != *shim->m_iid)
	{
		obj->IncRef();
		m_iid_map[*shim->m_iid] = obj;
	}

	m_iid_map[iid] = obj;

	return obj;
}

Omega::IObject* Omega::System::MetaInfo::Safe_Proxy_Owner::QueryInterface(const Omega::guid_t& iid)
{
	// Always return the same object for IObject
	if (iid == OMEGA_GUIDOF(IObject) ||
		iid == OMEGA_GUIDOF(ISafeProxy))
	{
		AddRef();
		return static_cast<ISafeProxy*>(this);
	}

	// See if we have it cached
	Safe_Proxy_Base* obj = GetBase(iid);
	if (!obj)
		return 0;

	AddRef();
	return obj->QIReturn();
}

void Omega::System::MetaInfo::Safe_Proxy_Owner::Throw(const Omega::guid_t& iid)
{
	// See if we have it cached
	Safe_Proxy_Base* obj = GetBase(iid);
	if (!obj)
		OMEGA_THROW(L"Failed to create safe proxy");

	AddRef();
	return obj->Throw();
}

Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Safe_Stub_Owner::CachedQI(Safe_Stub_Base* pStub, const guid_t& iid)
{
	Threading::Guard<Threading::Mutex> guard(m_lock);

	// See if we have it cached
	try
	{
		std::map<guid_t,SafeShim*>::iterator i=m_iid_map.find(iid);
		if (i == m_iid_map.end())
		{
			// See if we have a derived iid
			for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
			{
				if (static_cast<Safe_Stub_Base*>(i->second->m_stub)->IsDerived(iid))
				{
					m_iid_map[iid] = i->second;
					break;
				}
			}
		}

		if (i != m_iid_map.end())
		{
			static_cast<Safe_Stub_Base*>(i->second->m_stub)->AddRef();
			return i->second;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	// Check the actual interface pointer
	SafeShim* shim = pStub->ShimQI(iid);
	if (!shim)
		return 0;

	try
	{
		m_iid_map[iid] = shim;
	}
	catch (std::exception& e)
	{
		static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnRelease_Safe(shim);
		OMEGA_THROW(e);
	}

	return shim;
}

void Omega::System::MetaInfo::Safe_Stub_Owner::RemoveShim(SafeShim* shim)
{
	Threading::Guard<Threading::Mutex> guard(m_lock);

	for (std::map<guid_t,SafeShim*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();)
	{
		if (i->second == shim)
			m_iid_map.erase(i++);
		else
			++i;
	}

	if (m_iid_map.empty())
	{
		guard.Release();
		delete this;
	}
}

Omega::System::MetaInfo::Safe_Proxy_Base* Omega::System::MetaInfo::create_proxy_base(SafeShim* shim)
{
	// QI for the IObject shim
	SafeShim* base_shim = 0;
	SafeShim* except = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnQueryInterface_Safe(shim,&base_shim,&OMEGA_GUIDOF(IObject));
	if (except)
		throw_correct_exception(except);

	// Release the incoming shim
	except = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnRelease_Safe(shim);
	if (except)
		throw_correct_exception(except);

	// Lookup in the global map...
	Safe_Proxy_Base* obj = 0;

	if (!obj)
	{
		// Create a default proxy...
		obj = Safe_Proxy<IObject,IObject>::bind(base_shim,0);
		if (!obj)
			OMEGA_THROW(L"Failed to create safe proxy");
	}

	return obj;
}

Omega::IObject* Omega::System::MetaInfo::create_proxy(SafeShim* shim)
{
	if (!shim)
		return 0;

	// QI and return
	guid_t iid = *shim->m_iid;
	auto_iface_ptr<IObject> ptrObj = create_proxy_base(shim)->QIReturn();
	return ptrObj->QueryInterface(iid);
}

void Omega::System::MetaInfo::throw_correct_exception(SafeShim* shim)
{
	guid_t iid = *shim->m_iid;
	create_proxy_base(shim)->Throw(iid);
}

Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::create_stub(IObject* pObj, const guid_t& iid)
{
	if (!pObj)
		return 0;

	// See if pObj is actually a proxy...
	auto_iface_ptr<ISafeProxy> ptrProxy = static_cast<ISafeProxy*>(pObj->QueryInterface(OMEGA_GUIDOF(ISafeProxy)));
	if (ptrProxy)
		return ptrProxy->GetStub(iid);

	// Get the IObject interface
	auto_iface_ptr<IObject> ptrObject = static_cast<IObject*>(pObj->QueryInterface(OMEGA_GUIDOF(IObject)));

	// Lookup in the global map
	bool bReleaseShim = false;
	SafeShim* shim = 0;
	
	if (!shim)
	{
		// Create the IObject shim
		SafeShim* except = Safe_Stub<IObject>::create(ptrObject,0,&shim);
		if (except)
			throw_correct_exception(except);
		if (!shim)
			OMEGA_THROW(L"Failed to create safe stub");

		bReleaseShim = true;
	}

	// QI the base shim for the iid
	SafeShim* iface_shim = 0;
	SafeShim* except = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnQueryInterface_Safe(shim,&iface_shim,&iid);
	if (except)
		throw_correct_exception(except);

	if (bReleaseShim)
	{
		except = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnRelease_Safe(shim);
		if (except)
			throw_correct_exception(except);
	}

	return iface_shim;
}

Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::return_safe_exception(IException* pE)
{
	auto_iface_ptr<IException> ptrE(pE);
	return create_stub(pE,ptrE->GetThrownIID());
}

#endif // OOCORE_SAFE_INL_INCLUDED_
