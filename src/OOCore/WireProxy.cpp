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

#include "OOCore_precomp.h"

#include "WireProxy.h"
#include "StdObjectManager.h"

using namespace Omega;
using namespace OTL;

OOCore::Proxy::Proxy() :
	m_proxy_id(0)
{
	static const System::MetaInfo::vtable_info<Remoting::IProxy>::type proxy_vt =
	{
		{
			&AddRef_Safe,
			&Release_Safe,
			&QueryInterface_Safe,
			&Pin_Safe,
			&Unpin_Safe,
			0,
			0
		},
		&WriteKey_Safe,
		&UnpackKey_Safe,
		&GetMarshaller_Safe,
		&IsAlive_Safe,
		&RemoteQueryInterface_Safe
	};

	m_proxy_shim.m_vtable = &proxy_vt;
	m_proxy_shim.m_stub = this;
	m_proxy_shim.m_iid = &OMEGA_GUIDOF(Remoting::IProxy);

	static const System::MetaInfo::vtable_info<Remoting::IMarshal>::type marshal_vt =
	{
		{
			&AddRef_Safe,
			&Release_Safe,
			&QueryInterface_Safe,
			&Pin_Safe,
			&Unpin_Safe,
			0,
			0
		},
		&GetUnmarshalFactoryOID_Safe,
		&MarshalInterface_Safe,
		&ReleaseMarshalData_Safe
	};

	m_marshal_shim.m_vtable = &marshal_vt;
	m_marshal_shim.m_stub = this;
	m_marshal_shim.m_iid = &OMEGA_GUIDOF(Remoting::IMarshal);

	m_iids.insert(std::map<Omega::guid_t,bool_t>::value_type(OMEGA_GUIDOF(IObject),true));
}

OOCore::Proxy::~Proxy()
{
	CallRemoteRelease();

	m_pManager->RemoveProxy(m_proxy_id);
	static_cast<Remoting::IObjectManager*>(m_pManager)->Release();
}

void OOCore::Proxy::init(uint32_t proxy_id, StdObjectManager* pManager)
{
	m_proxy_id = proxy_id;
	m_pManager = pManager;
	static_cast<Remoting::IObjectManager*>(m_pManager)->AddRef();
}

void OOCore::Proxy::Disconnect()
{
	// Force our marshal count to 0, cos the other end has gone
	m_marshal_count = 0;
}

Remoting::IMarshaller* OOCore::Proxy::GetMarshaller()
{
	Remoting::IMarshaller* pRet = static_cast<Remoting::IMarshaller*>(m_pManager);
	pRet->AddRef();
	return pRet;
}

bool_t OOCore::Proxy::IsAlive()
{
	return m_pManager->IsAlive();
}

bool_t OOCore::Proxy::RemoteQueryInterface(const guid_t& iid)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	std::map<Omega::guid_t,bool>::const_iterator i = m_iids.find(iid);
	if (i != m_iids.end())
		return i->second;

	guard.release();

	ObjectPtr<Remoting::IMessage> pParamsOut;
	pParamsOut.Attach(m_pManager->CreateMessage());

	WriteStubInfo(pParamsOut,1);

	pParamsOut->WriteGuid(L"iid",iid);
	pParamsOut->WriteStructEnd(L"ipc_request");

	Remoting::IMessage* pParamsIn = 0;
	IException* pE = m_pManager->SendAndReceive(TypeInfo::Synchronous,pParamsOut,pParamsIn);
	if (pE)
		pE->Throw();

	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	bool_t bOk = ptrParamsIn->ReadBoolean(L"$retval");
	
	guard.acquire();

	m_iids.insert(std::map<Omega::guid_t,bool_t>::value_type(iid,bOk));
	
	return bOk;
}

IObject* OOCore::Proxy::UnmarshalInterface(Remoting::IMessage* pMessage, const guid_t& iid)
{
	// Up our marshal count early, because we are definitely attached to something!
	++m_marshal_count;

	guid_t wire_iid = pMessage->ReadGuid(L"iid");

	System::MetaInfo::auto_iface_ptr<System::MetaInfo::Wire_Proxy_Owner> ptrOwner = System::MetaInfo::create_wire_proxy_owner(&m_proxy_shim,0);

	return ptrOwner->CreateProxy(wire_iid,iid);
}

void OOCore::Proxy::WriteStubInfo(Remoting::IMessage* pMessage, uint32_t method_id)
{
	pMessage->WriteStructStart(L"ipc_request",L"$ipc_request_type");
	pMessage->WriteUInt32(L"$stub_id",m_proxy_id);
	pMessage->WriteGuid(L"$iid",OMEGA_GUIDOF(IObject));
	pMessage->WriteUInt32(L"$method_id",method_id);
}

void OOCore::Proxy::ReadStubInfo(Remoting::IMessage* pMessage)
{
	pMessage->ReadStructStart(L"ipc_request",L"$ipc_request_type");
	pMessage->ReadUInt32(L"$stub_id");
	pMessage->ReadGuid(L"$iid");
	pMessage->ReadUInt32(L"$method_id");
}

const System::MetaInfo::SafeShim* OOCore::Proxy::GetShim(const guid_t& iid)
{
	if (iid == OMEGA_GUIDOF(IObject) ||
		iid == OMEGA_GUIDOF(Remoting::IProxy))
	{
		Internal_AddRef();
		return &m_proxy_shim;
	}
	else if (iid == OMEGA_GUIDOF(Remoting::IMarshal))
	{
		Internal_AddRef();
		return &m_marshal_shim;
	}
	
	return 0;
}

Remoting::IMessage* OOCore::Proxy::CallRemoteStubMarshal(Remoting::IMarshaller* pMarshaller, const guid_t& iid)
{
	ObjectPtr<Remoting::IMessage> pParamsOut;
	pParamsOut.Attach(m_pManager->CreateMessage());

	WriteStubInfo(pParamsOut,2);

	pParamsOut->WriteGuid(L"iid",iid);

	Remoting::IMessage* pParamsIn = 0;
	
	try
	{
		m_pManager->DoMarshalChannel(pMarshaller,pParamsOut);

		pParamsOut->WriteStructEnd(L"ipc_request");

		IException* pE = m_pManager->SendAndReceive(TypeInfo::Synchronous,pParamsOut,pParamsIn);
		if (pE)
			pE->Throw();
	}
	catch (...)
	{
		ReadStubInfo(pParamsOut);

		pParamsOut->ReadGuid(L"iid");

		void* TODO; // Release marshal data for channel
		//m_pManager->ReleaseMarshalData(L"pObjectManager",pParamsOut,OMEGA_GUIDOF(Remoting::IMarshaller),pObjectManager);

		throw;
	}

	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	return ObjectPtr<Remoting::IMarshaller>(static_cast<Remoting::IMarshaller*>(m_pManager)).UnmarshalInterface<Remoting::IMessage>(L"pReflect",ptrParamsIn).AddRef();
}

void OOCore::Proxy::CallRemoteRelease()
{
	if (m_marshal_count == 0)
		return;

	try
	{
		ObjectPtr<Remoting::IMessage> pParamsOut;
		pParamsOut.Attach(m_pManager->CreateMessage());

		WriteStubInfo(pParamsOut,0);

		pParamsOut->WriteUInt32(L"release_count",static_cast<uint32_t>(m_marshal_count.value()));
		pParamsOut->WriteStructEnd(L"ipc_request");

		Remoting::IMessage* pParamsIn = 0;
		IException* pE = m_pManager->SendAndReceive(TypeInfo::Synchronous,pParamsOut,pParamsIn);

		if (pParamsIn)
			pParamsIn->Release();

		if (pE)
			pE->Release();
	}
	catch (IException* pE)
	{
		pE->Release();
	}
}

void OOCore::Proxy::MarshalInterface(Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t)
{
	// Tell the stub to expect incoming requests from a different channel...
	ObjectPtr<Remoting::IMessage> ptrReflect;
	ptrReflect.Attach(CallRemoteStubMarshal(pMarshaller,iid));

	return pMarshaller->MarshalInterface(L"pReflect",pMessage,OMEGA_GUIDOF(Remoting::IMessage),ptrReflect);
}

void OOCore::Proxy::ReleaseMarshalData(Remoting::IMarshaller*, Remoting::IMessage*, const guid_t&, Remoting::MarshalFlags_t)
{
	// How do we undo this?
	void* TODO;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::AddRef_Safe(const System::MetaInfo::SafeShim* shim)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Proxy*>(shim->m_stub)->Internal_AddRef();
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::Release_Safe(const System::MetaInfo::SafeShim* shim)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Proxy*>(shim->m_stub)->Internal_Release();
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::QueryInterface_Safe(const System::MetaInfo::SafeShim* shim, const System::MetaInfo::SafeShim** retval, const guid_base_t* iid)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		*retval = System::MetaInfo::create_safe_stub(static_cast<Proxy*>(shim->m_stub)->Internal_QueryInterface(*iid,getQIEntries()),*iid);
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::Pin_Safe(const System::MetaInfo::SafeShim* shim)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Proxy*>(shim->m_stub)->Pin();
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::Unpin_Safe(const System::MetaInfo::SafeShim* shim)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Proxy*>(shim->m_stub)->Unpin();
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::WriteKey_Safe(const System::MetaInfo::SafeShim* shim, const System::MetaInfo::SafeShim* pMessage)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Proxy*>(shim->m_stub)->WriteKey(System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pMessage));
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::UnpackKey_Safe(const System::MetaInfo::SafeShim* shim, const System::MetaInfo::SafeShim* pMessage)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Proxy*>(shim->m_stub)->UnpackKey(System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pMessage));
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::GetMarshaller_Safe(const System::MetaInfo::SafeShim* shim, const System::MetaInfo::SafeShim** retval)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Remoting::IMarshaller*&>(System::MetaInfo::marshal_info<Remoting::IMarshaller*&>::safe_type::coerce(retval)) = static_cast<Proxy*>(shim->m_stub)->GetMarshaller();
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::IsAlive_Safe(const System::MetaInfo::SafeShim* shim, int* retval)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<bool_t&>(System::MetaInfo::marshal_info<bool_t&>::safe_type::coerce(retval)) = static_cast<Proxy*>(shim->m_stub)->IsAlive();
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::RemoteQueryInterface_Safe(const System::MetaInfo::SafeShim* shim, int* retval, const Omega::guid_base_t* iid)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<bool_t&>(System::MetaInfo::marshal_info<bool_t&>::safe_type::coerce(retval)) = static_cast<Proxy*>(shim->m_stub)->RemoteQueryInterface(*iid);
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::GetUnmarshalFactoryOID_Safe(const System::MetaInfo::SafeShim* shim, guid_base_t* retval, const guid_base_t* piid, Remoting::MarshalFlags_t flags)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<guid_t&>(System::MetaInfo::marshal_info<guid_t&>::safe_type::coerce(retval)) = static_cast<Proxy*>(shim->m_stub)->GetUnmarshalFactoryOID(*piid,flags);
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::MarshalInterface_Safe(const System::MetaInfo::SafeShim* shim, const System::MetaInfo::SafeShim* pMarshaller, const System::MetaInfo::SafeShim* pMessage, const guid_base_t* iid, Remoting::MarshalFlags_t flags)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Proxy*>(shim->m_stub)->MarshalInterface(System::MetaInfo::marshal_info<Remoting::IMarshaller*>::safe_type::coerce(pMarshaller),System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pMessage),*iid,flags);
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::ReleaseMarshalData_Safe(const System::MetaInfo::SafeShim* shim, const System::MetaInfo::SafeShim* pMarshaller, const System::MetaInfo::SafeShim* pMessage, const guid_base_t* iid, Remoting::MarshalFlags_t flags)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Proxy*>(shim->m_stub)->ReleaseMarshalData(System::MetaInfo::marshal_info<Remoting::IMarshaller*>::safe_type::coerce(pMarshaller),System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pMessage),*iid,flags);
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

void OOCore::ProxyMarshalFactory::UnmarshalInterface(Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	// Unmarshal the reflect package
	ObjectPtr<Remoting::IMessage> ptrReflect = ObjectPtr<Remoting::IMarshaller>(pMarshaller).UnmarshalInterface<Remoting::IMessage>(L"pReflect",pMessage);
	if (!ptrReflect)
		OMEGA_THROW(L"No package");
	
	// Unmarshal the manager
	ObjectPtr<Remoting::IChannel> ptrChannel = ObjectPtr<Remoting::IMarshaller>(pMarshaller).UnmarshalInterface<Remoting::IChannel>(L"m_ptrChannel",ptrReflect);
	if (!ptrChannel)
		OMEGA_THROW(L"No channel");
		
	ObjectPtr<Remoting::IObjectManager> ptrOM;
	ptrOM.Attach(ptrChannel->GetObjectManager());

	// QI for IMarshaller
	ObjectPtr<Remoting::IMarshaller> ptrMarshaller(ptrOM);
	if (!ptrMarshaller)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller),OMEGA_SOURCE_INFO);

	// Unmarshal the new proxy on the new manager
	ptrMarshaller->UnmarshalInterface(L"stub",ptrReflect,iid,pObject);
}
