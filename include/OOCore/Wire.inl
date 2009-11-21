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

#ifndef OOCORE_WIRE_INL_INCLUDED_
#define OOCORE_WIRE_INL_INCLUDED_

void Omega::System::MetaInfo::wire_proxy_holder::remove(const SafeShim* shim)
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

Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Wire_Proxy_Owner> Omega::System::MetaInfo::wire_proxy_holder::find(const SafeShim* shim)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::map<const SafeShim*,Wire_Proxy_Owner*>::const_iterator i=m_map.find(shim);
		if (i != m_map.end())
		{
			i->second->AddRef();
			return auto_iface_ptr<Wire_Proxy_Owner>(i->second);
		}
		
		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Wire_Proxy_Owner> Omega::System::MetaInfo::wire_proxy_holder::add(const SafeShim* shim, Wire_Proxy_Owner* pOwner)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::pair<std::map<const SafeShim*,Wire_Proxy_Owner*>::iterator,bool> p = m_map.insert(std::map<const SafeShim*,Wire_Proxy_Owner*>::value_type(shim,pOwner));
		if (!p.second)
		{
			p.first->second->AddRef();
			return auto_iface_ptr<Wire_Proxy_Owner>(p.first->second);
		}
							
		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Omega::System::MetaInfo::Wire_Proxy_Owner::~Wire_Proxy_Owner()
{
	// QI for ISafeProxy 
	auto_iface_ptr<ISafeProxy> ptrSP = static_cast<ISafeProxy*>(m_ptrProxy->QueryInterface(OMEGA_GUIDOF(ISafeProxy)));
	assert(ptrSP);

	auto_safe_shim shim = ptrSP->GetShim(OMEGA_GUIDOF(IObject));
	assert(shim);

	// Get the base shim
	const SafeShim* base_shim;
	const SafeShim* except = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnGetBaseShim_Safe(shim,&base_shim);
	if (except)
		throw_correct_exception(except);

	auto_safe_shim ss_base = base_shim;

	WIRE_PROXY_HOLDER::instance()->remove(base_shim);
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Wire_Proxy_Base> Omega::System::MetaInfo::Wire_Proxy_Owner::GetProxyBase(const guid_t& iid, bool bAllowPartial, bool bQI)
{
	assert(iid != OMEGA_GUIDOF(IObject));
	assert(iid != OMEGA_GUIDOF(ISafeProxy));

	Threading::Guard<Threading::Mutex> guard(m_lock);

	try
	{
		// See if we have it cached
		std::map<guid_t,Wire_Proxy_Base*>::iterator i=m_iid_map.find(iid);
		if (i == m_iid_map.end())
		{
			// See if we have a derived iid
			for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
			{
				if (i->second->IsDerived__proxy__(iid))
				{
					m_iid_map.insert(std::map<guid_t,Wire_Proxy_Base*>::value_type(iid,i->second));
					break;
				}
			}
		}

		if (i != m_iid_map.end())
		{
			i->second->AddRef();
			return auto_iface_ptr<Wire_Proxy_Base>(i->second);
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	// QueryInterface the actual interface pointer
	if (bQI)
	{
		if (!m_ptrProxy->RemoteQueryInterface(iid))
			return 0;
	}

	// Wrap it in a proxy and add it...
	const wire_rtti* rtti = get_wire_rtti_info(iid);
	if (!rtti && bAllowPartial)
		rtti = get_wire_rtti_info(OMEGA_GUIDOF(IObject));

	if (!rtti)
		OMEGA_THROW(L"Failed to create wire proxy for interface - missing rtti");

	auto_iface_ptr<Wire_Proxy_Base> obj = (*rtti->pfnCreateWireProxy)(this);
	if (!obj)
		OMEGA_THROW(L"Failed to create wire proxy");

	try
	{
		m_iid_map.insert(std::map<guid_t,Wire_Proxy_Base*>::value_type(iid,obj));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	return obj;
}

void Omega::System::MetaInfo::Wire_Proxy_Owner::RemoveBase(Wire_Proxy_Base* pProxy)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		for (std::map<guid_t,Wire_Proxy_Base*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();)
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

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Wire_Proxy_Owner::GetShim(const guid_t& iid)
{
	if (iid == OMEGA_GUIDOF(IObject))
		return GetBaseShim();
	
	// See if we have it cached
	auto_iface_ptr<Wire_Proxy_Base> obj = GetProxyBase(iid,true,false);
	if (!obj)
		OMEGA_THROW(L"Failed to create wire proxy");

	// Return the shim
	return obj->GetShim();
}

Omega::IObject* Omega::System::MetaInfo::Wire_Proxy_Owner::QueryInterface(const guid_t& iid)
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
	
	// Try the outer proxy first... this might save a round-trip
	IObject* pOuter = m_ptrProxy->QueryInterface(iid);
	if (pOuter)
		return pOuter;
			
	// See if we have it cached
	auto_iface_ptr<Wire_Proxy_Base> obj = GetProxyBase(iid,false,true);
	if (!obj)
		return 0;
	
	// Return cast to the correct type
	return obj->QIReturn__proxy__();
}

Omega::IObject* Omega::System::MetaInfo::Wire_Proxy_Owner::CreateProxy(const guid_t& iid)
{
	if (iid == OMEGA_GUIDOF(IObject))
	{
		m_internal.AddRef();
		return &m_internal;
	}
		
	// See if we have it cached
	auto_iface_ptr<Wire_Proxy_Base> obj = GetProxyBase(iid,true,false);
	if (!obj)
		return 0;
	
	// Return cast to the correct type
	return obj->QIReturn__proxy__();
}

void Omega::System::MetaInfo::Wire_Proxy_Owner::Throw(const guid_t& iid)
{
	assert(iid != OMEGA_GUIDOF(ISafeProxy));
	assert(iid != OMEGA_GUIDOF(IObject));

	// See if we have it cached
	auto_iface_ptr<Wire_Proxy_Base> obj = GetProxyBase(iid,false,false);
	if (!obj)
	{
		obj = GetProxyBase(OMEGA_GUIDOF(IException),false,false);
		if (!obj)
			OMEGA_THROW(L"Failed to create wire exception proxy");
	}

	return obj->Throw__proxy__();
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Wire_Proxy_Owner::GetWireProxy()
{
	// We know that m_ptrProxy is a SafeProxy
	auto_iface_ptr<ISafeProxy> ptrSProxy = static_cast<ISafeProxy*>(m_ptrProxy->QueryInterface(OMEGA_GUIDOF(ISafeProxy)));
	if (!ptrSProxy)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(ISafeProxy));

	return ptrSProxy->GetShim(OMEGA_GUIDOF(Remoting::IProxy));
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::Remoting::IMessage> Omega::System::MetaInfo::Wire_Proxy_Owner::CreateMessage(Remoting::IMarshaller* pMarshaller, const guid_t& iid, uint32_t method_id)
{
	auto_iface_ptr<Remoting::IMessage> ptrMessage = pMarshaller->CreateMessage();
	bool unpack = false;
	try
	{
		ptrMessage->WriteStructStart(L"ipc_request",L"$ipc_request_type");
		unpack = true;
		m_ptrProxy->WriteKey(ptrMessage);
		ptrMessage->WriteGuid(L"$iid",iid);
		ptrMessage->WriteUInt32(L"$method_id",method_id);
		return ptrMessage;
	}
	catch (...)
	{
		if (unpack)
		{
			ptrMessage->ReadStructStart(L"ipc_request",L"$ipc_request_type");
			m_ptrProxy->UnpackKey(ptrMessage);
		}
		throw;
	}
}

void Omega::System::MetaInfo::Wire_Proxy_Owner::UnpackHeader(Remoting::IMessage* pMessage)
{
	pMessage->ReadStructStart(L"ipc_request",L"$ipc_request_type");
	m_ptrProxy->UnpackKey(pMessage);
	pMessage->ReadGuid(L"$iid");
	pMessage->ReadUInt32(L"$method_id");
}

Omega::System::MetaInfo::Wire_Proxy_Base::~Wire_Proxy_Base()
{
	m_pOwner->RemoveBase(this);
}

Omega::IObject* Omega::System::MetaInfo::Wire_Proxy_Base::QueryInterface(const guid_t& iid)
{
	if (iid == OMEGA_GUIDOF(ISafeProxy))
	{
		AddRef();
		return &m_internal;
	}
		
	return m_pOwner->QueryInterface(iid);
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Wire_Proxy_Base::GetShim(const guid_t& iid)
{
	if (IsDerived__proxy__(iid))
		return GetShim();
	
	return m_pOwner->GetShim(iid);
}

void Omega::System::MetaInfo::Wire_Proxy_Base::Throw(const guid_t& iid)
{
	m_pOwner->Throw(iid);
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Wire_Proxy_Base::GetBaseShim()
{
	return m_pOwner->GetBaseShim();
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Wire_Proxy_Base::GetWireProxy()
{
	return m_pOwner->GetWireProxy();
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::Remoting::IMarshaller> Omega::System::MetaInfo::Wire_Proxy_Base::GetMarshaller()
{
	return m_pOwner->GetMarshaller();
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::Remoting::IMessage> Omega::System::MetaInfo::Wire_Proxy_Base::CreateMessage(Remoting::IMarshaller* pMarshaller, const guid_t& iid, uint32_t method_id)
{
	return m_pOwner->CreateMessage(pMarshaller,iid,method_id);
}

void Omega::System::MetaInfo::Wire_Proxy_Base::UnpackHeader(Omega::Remoting::IMessage* pMessage)
{
	return m_pOwner->UnpackHeader(pMessage);
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Wire_Proxy_Owner> Omega::System::MetaInfo::create_wire_proxy_owner(const SafeShim* shim, IObject* pOuter)
{
	// QI for the IObject shim
	const SafeShim* base_shim;
	const SafeShim* except = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnGetBaseShim_Safe(shim,&base_shim);
	if (except)
		throw_correct_exception(except);

	auto_safe_shim ss_base = base_shim;

	// Lookup in the global map...
	auto_iface_ptr<Wire_Proxy_Owner> ptrOwner = WIRE_PROXY_HOLDER::instance()->find(base_shim);
	if (ptrOwner)
		return ptrOwner;
	
	// Create a wire proxy owner
	OMEGA_NEW(ptrOwner,Wire_Proxy_Owner(base_shim,pOuter));
	
	// Add to the map...
	auto_iface_ptr<Wire_Proxy_Owner> ptrExisting = WIRE_PROXY_HOLDER::instance()->add(base_shim,ptrOwner);
	if (ptrExisting)
		return ptrExisting;
	
	return ptrOwner;
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::create_wire_stub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid, IObject* pObj)
{
	// Check that pObj supports the interface...
	auto_iface_ptr<IObject> ptrQI(pObj->QueryInterface(iid));
	if (!ptrQI)
		throw INoInterfaceException::Create(iid);	

	// Proxy the incoming params
	auto_iface_ptr<Remoting::IStubController> ptrController = static_cast<Remoting::IStubController*>(create_safe_proxy(shim_Controller));
	auto_iface_ptr<Remoting::IMarshaller> ptrMarshaller = static_cast<Remoting::IMarshaller*>(create_safe_proxy(shim_Marshaller));

	// Wrap it in a proxy and add it...
	const wire_rtti* rtti = get_wire_rtti_info(iid);
	if (!rtti)
		OMEGA_THROW(L"Failed to create wire stub for interface - missing rtti");

	return (*rtti->pfnCreateWireStub)(ptrController,ptrMarshaller,ptrQI);
}

#if !defined(DOXYGEN)

OOCORE_EXPORTED_FUNCTION(Omega::Remoting::IMessage*,OOCore_Remoting_CreateMemoryMessage,0,());
Omega::Remoting::IMessage* Omega::Remoting::CreateMemoryMessage()
{
	return OOCore_Remoting_CreateMemoryMessage();
}

#endif // !defined(DOXYGEN)

#endif // OOCORE_WIRE_INL_INCLUDED_
