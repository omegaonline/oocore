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

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_rtti_holder__ctor,2,((in),void**,phandle,(in),Omega::Threading::SingletonCallback,pfn_init));
inline void* Omega::System::Internal::wire_rtti_holder::handle()
{
	static void* s_handle = NULL;
	OOCore_wire_rtti_holder__ctor(&s_handle,&init);
	return s_handle;
}

inline const Omega::System::Internal::SafeShim* OMEGA_CALL Omega::System::Internal::wire_rtti_holder::init(void** param)
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

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_rtti_holder__dctor,1,((in),void*,handle));
inline void Omega::System::Internal::wire_rtti_holder::destroy(void* param)
{
	OOCore_wire_rtti_holder__dctor(param);
}

OOCORE_RAW_EXPORTED_FUNCTION(const Omega::System::Internal::wire_rtti*,OOCore_wire_rtti_holder_find,2,((in),void*,handle,(in),const Omega::guid_base_t*,iid));
inline const Omega::System::Internal::wire_rtti* Omega::System::Internal::get_wire_rtti_info(const guid_t& iid)
{
	return OOCore_wire_rtti_holder_find(wire_rtti_holder::handle(),&iid);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_rtti_holder_insert,3,((in),void*,handle,(in),const Omega::guid_base_t*,iid,(in),const Omega::System::Internal::wire_rtti*,pRtti));
inline void Omega::System::Internal::register_wire_rtti_info(const guid_t& iid, const wire_rtti* pRtti)
{
	OOCore_wire_rtti_holder_insert(wire_rtti_holder::handle(),&iid,pRtti);
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_wire_holder__ctor,0,());
inline Omega::System::Internal::wire_holder::wire_holder() : m_handle(NULL)
{
	m_handle = OOCore_wire_holder__ctor();
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_holder__dctor,1,((in),void*,handle));
inline Omega::System::Internal::wire_holder::~wire_holder()
{
	OOCore_wire_holder__dctor(m_handle);
}

OOCORE_RAW_EXPORTED_FUNCTION(Omega::IObject*,OOCore_wire_holder_add,3,((in),void*,handle,(in),Omega::IObject*,pProxy,(in),Omega::IObject*,pObject));
inline Omega::IObject* Omega::System::Internal::wire_holder::add(IObject* pProxy, IObject* pObject)
{
	IObject* pRet = OOCore_wire_holder_add(m_handle,pProxy,pObject);
	if (pRet)
		pRet->AddRef();
		
	return pRet;
}

OOCORE_RAW_EXPORTED_FUNCTION(Omega::IObject*,OOCore_wire_holder_find,2,((in),void*,handle,(in),Omega::IObject*,pProxy));
inline Omega::IObject* Omega::System::Internal::wire_holder::find(IObject* pProxy)
{
	IObject* pRet = OOCore_wire_holder_find(m_handle,pProxy);
	if (pRet)
		pRet->AddRef();
		
	return pRet;
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_holder_remove,2,((in),void*,handle,(in),Omega::IObject*,pProxy));
inline void Omega::System::Internal::wire_holder::remove(IObject* pProxy)
{
	OOCore_wire_holder_remove(m_handle,pProxy);
}

inline Omega::System::Internal::auto_iface_ptr<Omega::Remoting::IMessage> Omega::System::Internal::Wire_Proxy_Base::CreateMessage(const guid_t& iid, uint32_t method_id)
{
	auto_iface_ptr<Remoting::IMessage> ptrMessage = m_ptrMarshaller->CreateMessage();
	bool unpack = false;
	try
	{
		ptrMessage->WriteStructStart(string_t::constant("ipc_request"),string_t::constant("$ipc_request_type"));
		unpack = true;
		m_ptrProxy->WriteKey(ptrMessage);
		ptrMessage->WriteValue(string_t::constant("$iid"),iid);
		ptrMessage->WriteValue(string_t::constant("$method_id"),method_id);
		return ptrMessage;
	}
	catch (...)
	{
		if (unpack)
		{
			ptrMessage->ReadStructStart(string_t::constant("ipc_request"),string_t::constant("$ipc_request_type"));
			m_ptrProxy->UnpackKey(ptrMessage);
		}
		throw;
	}
}

inline void Omega::System::Internal::Wire_Proxy_Base::UnpackHeader(Remoting::IMessage* pMessage)
{
	pMessage->ReadStructStart(string_t::constant("ipc_request"),string_t::constant("$ipc_request_type"));
	m_ptrProxy->UnpackKey(pMessage);
	pMessage->ReadValue(string_t::constant("$iid"));
	pMessage->ReadValue(string_t::constant("$method_id"));
}

inline Omega::IObject* Omega::System::Internal::Wire_Proxy_Base::QueryInterface(const guid_t& iid)
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
			return NULL;

		return create_wire_proxy(m_ptrProxy,iid);
	}
}

inline Omega::IException* Omega::System::Internal::Wire_Proxy_Base::Throw(const guid_t& iid)
{
	// Check the proxy supports the interface
	if (!m_ptrProxy->RemoteQueryInterface(iid))
		return NULL;

	return static_cast<IException*>(create_wire_proxy(m_ptrProxy,iid,OMEGA_GUIDOF(IException)));
}

inline Omega::IObject* Omega::System::Internal::create_wire_proxy(Remoting::IProxy* pProxy, const guid_t& iid, const guid_t& fallback_iid)
{
	IObject* obj = NULL;
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
		OMEGA_THROW("Failed to create proxy");

	return obj;
}

inline const Omega::System::Internal::SafeShim* Omega::System::Internal::Safe_Stub_Base::CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid)
{
	// Proxy the incoming params
	auto_iface_ptr<Remoting::IStubController> ptrController = create_safe_proxy<Remoting::IStubController>(shim_Controller);
	auto_iface_ptr<Remoting::IMarshaller> ptrMarshaller = create_safe_proxy<Remoting::IMarshaller>(shim_Marshaller);

	auto_iface_ptr<Remoting::IStub> ptrStub = create_wire_stub(ptrController,ptrMarshaller,iid,m_pI);

	return create_safe_stub(ptrStub,OMEGA_GUIDOF(Remoting::IStub));
}

inline Omega::Remoting::IStub* Omega::System::Internal::create_wire_stub(Remoting::IStubController* pController, Remoting::IMarshaller* pMarshaller, const guid_t& iid, IObject* pObj)
{
	Remoting::IStub* pStub = NULL;
	if (iid == OMEGA_GUIDOF(IObject))
	{
		pStub = Wire_Stub<IObject>::create(pController,pMarshaller,pObj);
	}
	else
	{
		// Check that pObj supports the interface...
		auto_iface_ptr<IObject> ptrQI = pObj->QueryInterface(iid);
		if (!ptrQI)
			return NULL;

		// Wrap it in a proxy and add it...
		const wire_rtti* rtti = get_wire_rtti_info(iid);
		if (!rtti)
			OMEGA_THROW("Failed to create wire stub for interface - missing rtti");

		pStub = (*rtti->pfnCreateWireStub)(pController,pMarshaller,ptrQI);
	}

	if (!pStub)
		OMEGA_THROW("Failed to create wire stub for interface");

	return pStub;
}

#if !defined(DOXYGEN)

OOCORE_EXPORTED_FUNCTION(Omega::Remoting::IMessage*,OOCore_Remoting_CreateMemoryMessage,0,());
inline Omega::Remoting::IMessage* Omega::Remoting::CreateMemoryMessage()
{
	return OOCore_Remoting_CreateMemoryMessage();
}

#endif // !defined(DOXYGEN)

#endif // OOCORE_WIRE_INL_INCLUDED_
