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

Omega::IObject* Omega::System::MetaInfo::Safe_Proxy_Owner::QueryInterface(const guid_t& iid)
{
	Threading::Guard<Threading::Mutex> guard(m_lock);

	// Always return the same object for IObject
	if (iid == OMEGA_GUIDOF(IObject) ||
		iid == OMEGA_GUIDOF(ISafeProxy))
	{
		AddRef();
		return static_cast<ISafeProxy*>(this);
	}

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
				break;
			}
		}
	}

	if (i != m_iid_map.end())
	{
		AddRef();
		i->second->IncRef();
		return i->second->QIReturn();
	}

	// QueryInterface the actual interface pointer
	SafeShim* shim = 0;
	SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnQueryInterface_Safe(m_shim,&shim,iid);
	if (except)
		throw_safe_exception(except);

	if (!shim)
		return 0;

	// Wrap it in a proxy and add it...
	const qi_rtti* rtti = get_qi_rtti_info(shim->iid);
	if (!rtti)
		rtti = get_qi_rtti_info(iid);
		
	if (!rtti)
		OMEGA_THROW(L"Failed to create proxy for interface - missing rtti");

	Safe_Proxy_Base* obj = (*rtti->pfnSafeProxyBind)(shim,this);
	if (!obj)
		OMEGA_THROW(L"Failed to create safe proxy");

	if (iid != shim->m_iid)
	{
		obj->IncRef();
		m_iid_map[shim->m_iid] = obj;
		AddRef();
	}

	m_iid_map[iid] = obj;
	AddRef();

	return obj->QIReturn();
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
	SafeShim* shim = pBase->ShimQI(iid);
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
		delete this;
}

void Omega::System::MetaInfo::throw_correct_exception(SafeShim* except)
{
	void* TODO;

	IException* pE = MetaInfo<IException>::proxy_factory::bind(except,0);
	throw static_cast<IException*>(pE);
}

Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::return_safe_exception(IException* pE)
{
	void* TODO;

	// TODO - Use the rtti info to find the most derived exception and return that
	SafeShim* except = MetaInfo<IException>::stub_factory::create(pE);
	pE->Release();
	return except;
}

#endif // OOCORE_SAFE_INL_INCLUDED_
