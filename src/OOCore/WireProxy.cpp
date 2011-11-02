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

void OOCore::Proxy::Final_Release()
{
	CallRemoteRelease();
	
	m_pManager->RemoveProxy(m_proxy_id);
	
	static_cast<Remoting::IObjectManager*>(m_pManager)->Release();

	delete this;
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

	if (iid == OMEGA_GUIDOF(IObject))
		return false;

	bool b;
	if (m_iids.find(iid,b))
		return b;
	
	guard.release();

	ObjectPtr<Remoting::IMessage> ptrParamsOut = m_pManager->CreateMessage();

	WriteStubInfo(ptrParamsOut,0);

	ptrParamsOut->WriteValue(L"iid",iid);
	ptrParamsOut->WriteStructEnd();

	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	IException* pE = m_pManager->SendAndReceive(TypeInfo::Synchronous,ptrParamsOut,ptrParamsIn);
	if (pE)
		pE->Rethrow();

	bool bOk = ptrParamsIn->ReadValue(L"$retval").cast<bool_t>();

	guard.acquire();

	bool* pv = m_iids.find(iid);
	if (pv)
		*pv = bOk;
	else
	{
		int err = m_iids.insert(iid,bOk);
		if (err != 0)
			OMEGA_THROW(err);
	}

	return bOk;
}

IObject* OOCore::Proxy::QueryIObject()
{
	ObjectPtr<Remoting::IMessage> ptrParamsOut = m_pManager->CreateMessage();

	WriteStubInfo(ptrParamsOut,1);

	ptrParamsOut->WriteStructEnd();

	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	IException* pE = m_pManager->SendAndReceive(TypeInfo::Synchronous,ptrParamsOut,ptrParamsIn);
	if (pE)
		pE->Rethrow();

	IObject* pRet = NULL;
	m_pManager->UnmarshalInterface(L"$retval",ptrParamsIn,OMEGA_GUIDOF(IObject),pRet);
	return pRet;
}

IObject* OOCore::Proxy::UnmarshalInterface(Remoting::IMessage* pMessage)
{
	guid_t iid = pMessage->ReadValue(L"iid").cast<guid_t>();

	// Add to the cache map...
	{
		OOBase::Guard<OOBase::SpinLock> guard(m_lock);

		bool* pv = m_iids.find(OMEGA_GUIDOF(IObject));
		if (pv)
			*pv = true;
		else
		{
			int err = m_iids.insert(OMEGA_GUIDOF(IObject),true);
			if (err != 0)
				OMEGA_THROW(err);
		}

		pv = m_iids.find(iid);
		if (pv)
			*pv = true;
		else
		{
			int err = m_iids.insert(iid,true);
			if (err != 0)
				OMEGA_THROW(err);
		}
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
	ObjectPtr<Remoting::IMessage> ptrParamsOut = m_pManager->CreateMessage();

	WriteStubInfo(ptrParamsOut,2);

	ptrParamsOut->WriteValue(L"iid",iid);

	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	IException* pERet = NULL;

	try
	{
		m_pManager->DoMarshalChannel(pMarshaller,ptrParamsOut);
		ptrParamsOut->WriteStructEnd();

		pERet = m_pManager->SendAndReceive(TypeInfo::Synchronous,ptrParamsOut,ptrParamsIn);
	}
	catch (...)
	{
		ReadStubInfo(ptrParamsOut);
		ptrParamsOut->ReadValue(L"iid");
		m_pManager->UndoMarshalChannel(pMarshaller,ptrParamsOut);	
		throw;
	}

	if (pERet)
		pERet->Rethrow();

	Omega::IObject* pObj = NULL;
	m_pManager->UnmarshalInterface(L"pReflect",ptrParamsIn,OMEGA_GUIDOF(Remoting::IMessage),pObj);
	return static_cast<Remoting::IMessage*>(pObj);
}

void OOCore::Proxy::CallRemoteRelease()
{
	try
	{
		ObjectPtr<Remoting::IMessage> ptrParamsOut = m_pManager->CreateMessage();

		WriteStubInfo(ptrParamsOut,uint32_t(-1));

		ptrParamsOut->WriteStructEnd();

		ObjectPtr<Remoting::IMessage> ptrParamsIn;
		ObjectPtr<IException> ptrE = m_pManager->SendAndReceive(TypeInfo::Synchronous,ptrParamsOut,ptrParamsIn);		
	}
	catch (IException* pE)
	{
		// Absorb all exceptions...
		pE->Release();
	}
}

void OOCore::Proxy::MarshalInterface(Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t)
{
	// Tell the stub to expect incoming requests from a different channel...
	ObjectPtr<Remoting::IMessage> ptrReflect = CallRemoteStubMarshal(pMarshaller,iid);

	return pMarshaller->MarshalInterface(L"pReflect",pMessage,OMEGA_GUIDOF(Remoting::IMessage),ptrReflect);
}

void OOCore::Proxy::ReleaseMarshalData(Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t flags)
{
	// Undo the effects of CallRemoteStubMarshal() by reading the new proxy and releasing it...
	ObjectPtr<Remoting::IMarshalFactory> ptrMF(OID_ProxyMarshalFactory);

	IObject* pObject = NULL;
	ptrMF->UnmarshalInterface(pMarshaller,pMessage,iid,flags,pObject);
	if (pObject)
		pObject->Release();
}

void OOCore::ProxyMarshalFactory::UnmarshalInterface(Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	// Unmarshal the reflect package
	ObjectPtr<Remoting::IMessage> ptrReflect;
	ptrReflect.Unmarshal(pMarshaller,L"pReflect",pMessage);
	if (!ptrReflect)
		OMEGA_THROW("No package");

	// Unmarshal the manager
	ObjectPtr<Remoting::IChannel> ptrChannel;
	ptrChannel.Unmarshal(pMarshaller,L"m_ptrChannel",ptrReflect);
	if (!ptrChannel)
		OMEGA_THROW("No channel");

	// Get the IMarshaller
	IObject* pObj = NULL;
	ptrChannel->GetManager(OMEGA_GUIDOF(Remoting::IMarshaller),pObj);
	ObjectPtr<Remoting::IMarshaller> ptrMarshaller = static_cast<Remoting::IMarshaller*>(pObj);
	if (!ptrMarshaller)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller));

	// Unmarshal the new proxy on the new manager
	ptrMarshaller->UnmarshalInterface(L"stub",ptrReflect,iid,pObject);
}
