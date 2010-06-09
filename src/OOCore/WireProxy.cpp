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

	WriteStubInfo(pParamsOut,0);

	pParamsOut->WriteValue(L"iid",iid);
	pParamsOut->WriteStructEnd();

	Remoting::IMessage* pParamsIn = 0;
	IException* pE = m_pManager->SendAndReceive(TypeInfo::Synchronous,pParamsOut,pParamsIn);
	if (pE)
		pE->Rethrow();

	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	bool bOk = ptrParamsIn->ReadValue(L"$retval").cast<bool_t>();

	guard.acquire();

	m_iids.insert(std::map<Omega::guid_t,bool_t>::value_type(iid,bOk));

	return bOk;
}

IObject* OOCore::Proxy::QueryIObject()
{
	ObjectPtr<Remoting::IMessage> pParamsOut;
	pParamsOut.Attach(m_pManager->CreateMessage());

	WriteStubInfo(pParamsOut,1);

	pParamsOut->WriteStructEnd();

	Remoting::IMessage* pParamsIn = 0;
	IException* pE = m_pManager->SendAndReceive(TypeInfo::Synchronous,pParamsOut,pParamsIn);
	if (pE)
		pE->Rethrow();

	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	IObject* pRet = 0;
	m_pManager->UnmarshalInterface(L"$retval",ptrParamsIn,OMEGA_GUIDOF(IObject),pRet);
	return pRet;
}

IObject* OOCore::Proxy::UnmarshalInterface(Remoting::IMessage* pMessage)
{
	guid_t iid = pMessage->ReadValue(L"iid").cast<guid_t>();

	// Add to the cache map...
	{
		OOBase::Guard<OOBase::SpinLock> guard(m_lock);

		m_iids.insert(std::map<Omega::guid_t,bool>::value_type(OMEGA_GUIDOF(IObject),true));
		m_iids.insert(std::map<Omega::guid_t,bool>::value_type(iid,true));
	}

	// Create a wire_proxy...
	return System::Internal::create_wire_proxy(this,iid);
}

void OOCore::Proxy::WriteStubInfo(Remoting::IMessage* pMessage, uint32_t method_id)
{
	pMessage->WriteStructStart(L"ipc_request",L"$ipc_request_type");
	pMessage->WriteValue(L"$stub_id",m_proxy_id);
	pMessage->WriteValue(L"$iid",OMEGA_GUIDOF(IObject));
	pMessage->WriteValue(L"$method_id",method_id);
}

void OOCore::Proxy::ReadStubInfo(Remoting::IMessage* pMessage)
{
	pMessage->ReadStructStart(L"ipc_request",L"$ipc_request_type");
	pMessage->ReadValue(L"$stub_id");
	pMessage->ReadValue(L"$iid");
	pMessage->ReadValue(L"$method_id");
}

Remoting::IMessage* OOCore::Proxy::CallRemoteStubMarshal(Remoting::IMarshaller* pMarshaller, const guid_t& iid)
{
	ObjectPtr<Remoting::IMessage> pParamsOut;
	pParamsOut.Attach(m_pManager->CreateMessage());

	WriteStubInfo(pParamsOut,2);

	pParamsOut->WriteValue(L"iid",iid);

	Remoting::IMessage* pParamsIn = 0;

	try
	{
		m_pManager->DoMarshalChannel(pMarshaller,pParamsOut);

		pParamsOut->WriteStructEnd();

		IException* pE = m_pManager->SendAndReceive(TypeInfo::Synchronous,pParamsOut,pParamsIn);
		if (pE)
			pE->Rethrow();
	}
	catch (...)
	{
		ReadStubInfo(pParamsOut);

		pParamsOut->ReadValue(L"iid");

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
	try
	{
		ObjectPtr<Remoting::IMessage> pParamsOut;
		pParamsOut.Attach(m_pManager->CreateMessage());

		WriteStubInfo(pParamsOut,uint32_t(-1));

		pParamsOut->WriteStructEnd();

		Remoting::IMessage* pParamsIn = 0;
		IException* pE = m_pManager->SendAndReceive(TypeInfo::Synchronous,pParamsOut,pParamsIn);

		if (pParamsIn)
			pParamsIn->Release();

		// Absord all exceptions...
		if (pE)
			pE->Release();
	}
	catch (IException* pE)
	{
		// Absord all exceptions...
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

void OOCore::ProxyMarshalFactory::UnmarshalInterface(Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	ObjectPtr<Remoting::IMarshaller> ptrMarshaller(pMarshaller);

	// Unmarshal the reflect package
	ObjectPtr<Remoting::IMessage> ptrReflect = ptrMarshaller.UnmarshalInterface<Remoting::IMessage>(L"pReflect",pMessage);
	if (!ptrReflect)
		OMEGA_THROW(L"No package");

	// Unmarshal the manager
	ObjectPtr<Remoting::IChannel> ptrChannel = ptrMarshaller.UnmarshalInterface<Remoting::IChannel>(L"m_ptrChannel",ptrReflect);
	if (!ptrChannel)
		OMEGA_THROW(L"No channel");

	// Get the IMarshaller
	ObjectPtr<Remoting::IMarshaller> ptrMarshaller2 = ptrChannel.GetManager<Remoting::IMarshaller>();
	if (!ptrMarshaller2)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller));

	// Unmarshal the new proxy on the new manager
	ptrMarshaller2->UnmarshalInterface(L"stub",ptrReflect,iid,pObject);
}
