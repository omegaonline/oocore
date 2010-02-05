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

Omega::IObject* Omega::System::MetaInfo::wire_holder::add(IObject* pProxy, IObject* pObject)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::pair<std::map<IObject*,IObject*>::iterator,bool> p = m_map.insert(std::map<IObject*,IObject*>::value_type(pProxy,pObject));
		if (!p.second)
		{
			p.first->second->AddRef();
			return p.first->second;
		}
							
		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Omega::IObject* Omega::System::MetaInfo::wire_holder::find(IObject* pProxy)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		std::map<IObject*,IObject*>::const_iterator i=m_map.find(pProxy);
		if (i != m_map.end())
		{
			i->second->AddRef();
			return i->second;
		}
		
		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void Omega::System::MetaInfo::wire_holder::remove(IObject* pProxy)
{
	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);
		m_map.erase(pProxy);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Omega::System::MetaInfo::auto_iface_ptr<Omega::Remoting::IMessage> Omega::System::MetaInfo::Wire_Proxy_Base::CreateMessage(const guid_t& iid, uint32_t method_id)
{
	auto_iface_ptr<Remoting::IMessage> ptrMessage = m_ptrMarshaller->CreateMessage();
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

void Omega::System::MetaInfo::Wire_Proxy_Base::UnpackHeader(Remoting::IMessage* pMessage)
{
	pMessage->ReadStructStart(L"ipc_request",L"$ipc_request_type");
	m_ptrProxy->UnpackKey(pMessage);
	pMessage->ReadGuid(L"$iid");
	pMessage->ReadUInt32(L"$method_id");
}

Omega::IObject* Omega::System::MetaInfo::Wire_Proxy_Base::QueryInterface(const guid_t& iid)
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

	if (iid == OMEGA_GUIDOF(IObject))
	{
		// Get the controlling IObject
		return m_ptrProxy->QueryIObject();
	}
	else
	{
		// Check the proxy supports the interface
		if (!m_ptrProxy->RemoteQueryInterface(iid))
			return 0;

		return create_wire_proxy(m_ptrProxy,iid);
	}
}

Omega::IException* Omega::System::MetaInfo::Wire_Proxy_Base::Throw(const guid_t& iid)
{
	// Check the proxy supports the interface
	if (!m_ptrProxy->RemoteQueryInterface(iid))
		return 0;

	return static_cast<IException*>(create_wire_proxy(m_ptrProxy,iid,OMEGA_GUIDOF(IException)));
}

Omega::IObject* Omega::System::MetaInfo::create_wire_proxy(Remoting::IProxy* pProxy, const guid_t& iid, const guid_t& fallback_iid)
{
	assert(iid != OMEGA_GUIDOF(ISafeProxy));
	assert(pProxy);
	
	IObject* obj = 0;
	if (iid == OMEGA_GUIDOF(IObject))
	{
		obj = Wire_Proxy_IObject::bind(pProxy);
	}
	else
	{
		// Find rtti...
		const wire_rtti* rtti = get_wire_rtti_info(iid);
		if (!rtti)
		{
			rtti = get_wire_rtti_info(fallback_iid);
			if (!rtti)
				rtti = get_wire_rtti_info(OMEGA_GUIDOF(IObject));
		}
			
		obj = (*rtti->pfnCreateWireProxy)(pProxy);
	}

	if (!obj)
		OMEGA_THROW(L"Failed to create proxy");

	return obj;
}

const Omega::System::MetaInfo::SafeShim* Omega::System::MetaInfo::Safe_Stub_Base::CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid)
{
	// Proxy the incoming params
	auto_iface_ptr<Remoting::IStubController> ptrController = create_safe_proxy<Remoting::IStubController>(shim_Controller);
	auto_iface_ptr<Remoting::IMarshaller> ptrMarshaller = create_safe_proxy<Remoting::IMarshaller>(shim_Marshaller);

	auto_iface_ptr<Remoting::IStub> ptrStub = create_wire_stub(ptrController,ptrMarshaller,iid,m_pI);

	return create_safe_stub(ptrStub,OMEGA_GUIDOF(Remoting::IStub));
}

Omega::Remoting::IStub* Omega::System::MetaInfo::create_wire_stub(Remoting::IStubController* pController, Remoting::IMarshaller* pMarshaller, const guid_t& iid, IObject* pObj)
{
	Remoting::IStub* pStub = 0;
	if (iid == OMEGA_GUIDOF(IObject))
	{
		pStub = Wire_Stub<IObject>::create(pController,pMarshaller,pObj);
	}
	else
	{
		// Check that pObj supports the interface...
		auto_iface_ptr<IObject> ptrQI = pObj->QueryInterface(iid);
		if (!ptrQI)
			return 0;
	
		// Wrap it in a proxy and add it...
		const wire_rtti* rtti = get_wire_rtti_info(iid);
		if (!rtti)
			OMEGA_THROW(L"Failed to create wire stub for interface - missing rtti");

		pStub = (*rtti->pfnCreateWireStub)(pController,pMarshaller,ptrQI);
	}

	if (!pStub)
		OMEGA_THROW(L"Failed to create wire stub for interface");

	return pStub;
}

#if !defined(DOXYGEN)

OOCORE_EXPORTED_FUNCTION(Omega::Remoting::IMessage*,OOCore_Remoting_CreateMemoryMessage,0,());
Omega::Remoting::IMessage* Omega::Remoting::CreateMemoryMessage()
{
	return OOCore_Remoting_CreateMemoryMessage();
}

#endif // !defined(DOXYGEN)

#endif // OOCORE_WIRE_INL_INCLUDED_
