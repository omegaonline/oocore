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

Omega::IObject* Omega::System::MetaInfo::safe_holder::add(const SafeShim* shim, IObject* pObject)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::pair<std::map<const SafeShim*,IObject*>::iterator,bool> p1 = m_shim_map.insert(std::map<const SafeShim*,IObject*>::value_type(shim,pObject));
		if (!p1.second)
		{
			p1.first->second->AddRef();
			return p1.first->second;
		}

		std::pair<std::map<IObject*,const SafeShim*>::iterator,bool> p2 = m_obj_map.insert(std::map<IObject*,const SafeShim*>::value_type(pObject,shim));
		assert(p2.second);

		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::safe_holder::add(IObject* pObject, const Omega::System::MetaInfo::SafeShim* shim)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::pair<std::map<IObject*,const SafeShim*>::iterator,bool> p1 = m_obj_map.insert(std::map<IObject*,const SafeShim*>::value_type(pObject,shim));
		if (!p1.second)
		{
			addref_safe(p1.first->second);
			return p1.first->second;
		}

		std::pair<std::map<const SafeShim*,IObject*>::iterator,bool> p2 = m_shim_map.insert(std::map<const SafeShim*,IObject*>::value_type(shim,pObject));
		assert(p2.second);

		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::safe_holder::find(IObject* pObject)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::map<IObject*,const SafeShim*>::const_iterator i=m_obj_map.find(pObject);
		if (i != m_obj_map.end())
		{
			addref_safe(i->second);
			return i->second;
		}

		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void Omega::System::MetaInfo::safe_holder::remove(IObject* pObject)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::map<IObject*,const SafeShim*>::iterator i=m_obj_map.find(pObject);
		if (i != m_obj_map.end())
		{
			m_shim_map.erase(i->second);
			m_obj_map.erase(i);
		}			
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void Omega::System::MetaInfo::safe_holder::remove(const SafeShim* shim)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::map<const SafeShim*,IObject*>::iterator i=m_shim_map.find(shim);
		if (i != m_shim_map.end())
		{
			m_obj_map.erase(i->second);
			m_shim_map.erase(i);
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Omega::IObject* Omega::System::MetaInfo::Safe_Proxy_Base::QueryInterface(const guid_t& iid)
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
	if (except)
		throw_correct_exception(except);

	return create_safe_proxy(retval,iid);
}

Omega::IObject* Omega::System::MetaInfo::create_safe_proxy(const SafeShim* shim, const guid_t& iid)
{
	if (!shim)
		return 0;

	// See if we are a Wire Proxy
	if (static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnGetWireProxy_Safe)
	{
		// Retrieve the underlying proxy
		auto_safe_shim proxy;
		const SafeShim* pE = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnGetWireProxy_Safe(shim,&proxy);
		if (pE)
			throw_correct_exception(pE);

		// Control its lifetime
		auto_iface_ptr<Remoting::IProxy> ptrProxy = create_safe_proxy<Remoting::IProxy>(proxy);

		assert(ptrProxy->RemoteQueryInterface(iid));
			
		// Create a wire proxy
		return create_wire_proxy(ptrProxy,iid);
	}
	
	IObject* obj = 0;
	if (guid_t(*shim->m_iid) == OMEGA_GUIDOF(IObject))
	{
		// Shims should always be 'complete'
		assert(iid == OMEGA_GUIDOF(IObject));
		obj = Safe_Proxy_IObject::bind(shim);
	}
	else
	{
		// Find the rtti info...
		const qi_rtti* rtti = get_qi_rtti_info(*shim->m_iid);
		if (!rtti && guid_t(*shim->m_iid) != iid)
			rtti = get_qi_rtti_info(iid);

		// Fall back to IObject for completely unknown interfaces
		if (!rtti)
			rtti = get_qi_rtti_info(OMEGA_GUIDOF(IObject));
			
		obj = (*rtti->pfnCreateSafeProxy)(shim,iid);
	}

	if (!obj)
		OMEGA_THROW(L"Failed to create proxy");

	return obj;
}

void Omega::System::MetaInfo::throw_correct_exception(const SafeShim* shim)
{
	assert(shim);

	// Ensure shim is released
	auto_safe_shim ss = shim;

	create_safe_proxy<IException>(shim)->Throw();
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::create_safe_stub(IObject* pObj, const guid_t& iid)
{
	if (!pObj)
		return 0;

	const SafeShim* shim = 0;

	// See if we have it cached...
	if (iid == OMEGA_GUIDOF(IObject))
	{
		shim = SAFE_HOLDER::instance()->find(pObj);
		if (shim)
			return shim;
	}	

	// See if pObj is actually a proxy...
	auto_iface_ptr<ISafeProxy> ptrProxy = static_cast<ISafeProxy*>(pObj->QueryInterface(OMEGA_GUIDOF(ISafeProxy)));
	if (ptrProxy)
	{
		shim = ptrProxy->GetShim(iid);
		if (shim)
			return shim;
	}

	// Return the special case for IObject
	if (iid == OMEGA_GUIDOF(IObject))
		shim = Safe_Stub_IObject::create(pObj);
	else
	{
		// Find the rtti info...
		const qi_rtti* rtti = get_qi_rtti_info(iid);
		if (!rtti)
			OMEGA_THROW(L"Failed to create stub for interface - missing rtti");

		shim = (*rtti->pfnCreateSafeStub)(pObj);
	}

	if (!shim)
		OMEGA_THROW(L"Failed to create safe stub");

	return shim;
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::return_safe_exception(IException* pE)
{
	auto_iface_ptr<IException> ptrE(pE);
	return create_safe_stub(pE,pE->GetThrownIID());
}

#endif // OOCORE_SAFE_INL_INCLUDED_
