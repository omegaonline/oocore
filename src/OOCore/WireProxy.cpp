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
#include "WireImpl.h"

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
}

void OOCore::Proxy::init(uint32_t proxy_id, StdObjectManager* pManager)
{
	m_proxy_id = proxy_id;
	m_pManager = pManager;
}

void OOCore::Proxy::Disconnect()
{
	// Force our marshal count to 0, cos the other end has gone
	m_marshal_count = 0;
}

System::IMarshaller* OOCore::Proxy::GetMarshaller()
{
	Omega::System::IMarshaller* pRet = static_cast<Omega::System::IMarshaller*>(m_pManager);
	pRet->AddRef();
	return pRet;
}

bool_t OOCore::Proxy::IsAlive()
{
	return m_pManager->IsAlive();
}

ObjectPtr<IObject> OOCore::Proxy::UnmarshalInterface(Remoting::IMessage* pMessage, const guid_t& iid)
{
	try
	{
		// Up our marshal count early, because we are definitely attached to something!
		++m_marshal_count;

		guid_t wire_iid;
		System::MetaInfo::wire_read(L"iid",pMessage,wire_iid);
		
		ObjectPtr<IObject> ptrProxy;

		// See if we have a proxy for this interface already...
		{
			OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

			std::map<const guid_t,ObjectPtr<IObject> >::iterator i=m_iid_map.find(wire_iid);
			if (i != m_iid_map.end())
				return i->second;
			
			// See if any known interface supports the new interface
			for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
			{		
				IObject* pQI = i->second->QueryInterface(wire_iid);
				if (pQI)
				{
					ptrProxy.Attach(pQI);
					break;
				}
			}
		}

		if (!ptrProxy)
		{
			// Create a new proxy for this interface
			ptrProxy.Attach(OOCore::CreateProxy(wire_iid,this));

			void* TODO; // We must do some validation here for wire_iid and iid...
		}

		OOBase::Guard<OOBase::RWMutex> guard(m_lock);
			
		std::pair<std::map<const guid_t,ObjectPtr<IObject> >::iterator,bool> p=m_iid_map.insert(std::map<const guid_t,ObjectPtr<IObject> >::value_type(wire_iid,ptrProxy));
		if (!p.second)
			ptrProxy = p.first->second;
		
		return ptrProxy;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void OOCore::Proxy::WriteStubInfo(Remoting::IMessage* pMessage, uint32_t method_id)
{
	pMessage->WriteStructStart(L"ipc_request",L"$ipc_request_type");

	System::MetaInfo::wire_write(L"$stub_id",pMessage,m_proxy_id);
	System::MetaInfo::wire_write(L"$iid",pMessage,OMEGA_GUIDOF(IObject));
	System::MetaInfo::wire_write(L"$method_id",pMessage,method_id);
}

void OOCore::Proxy::ReadStubInfo(Remoting::IMessage* pMessage)
{
	pMessage->ReadStructStart(L"ipc_request",L"$ipc_request_type");

	uint32_t l;
	guid_t m;
	System::MetaInfo::wire_read(L"$stub_id",pMessage,l);
	System::MetaInfo::wire_read(L"$iid",pMessage,m);
	System::MetaInfo::wire_read(L"$method_id",pMessage,l);
}

bool OOCore::Proxy::CallRemoteQI(const guid_t& iid)
{
	ObjectPtr<Remoting::IMessage> pParamsOut;
	pParamsOut.Attach(m_pManager->CreateMessage());

	WriteStubInfo(pParamsOut,1);

	System::MetaInfo::wire_write(L"iid",pParamsOut,iid);

	pParamsOut->WriteStructEnd(L"ipc_request");

	Remoting::IMessage* pParamsIn = 0;
	IException* pE = m_pManager->SendAndReceive(TypeInfo::Synchronous,pParamsOut,pParamsIn);
	
	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	if (pE)
		throw pE;

	bool_t retval;
	System::MetaInfo::wire_read(L"$retval",ptrParamsIn,retval);
	return retval;
}

IObject* OOCore::Proxy::QI(const guid_t& iid)
{
	ObjectPtr<IObject> ptrProxy;
		
	try
	{
		// See if we have a proxy for this interface already...
		{
			OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

			std::map<const guid_t,ObjectPtr<IObject> >::iterator i=m_iid_map.find(iid);
			if (i != m_iid_map.end())
				return i->second.AddRef();
			
			// See if any known interface supports the new interface
			for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
			{		
				IObject* pQI = i->second->QueryInterface(iid);
				if (pQI)
				{
					ptrProxy.Attach(pQI);
					break;
				}
			}
		}

		if (!ptrProxy)
		{
			// Send a packet to the other end to see if the stub supports the interface
			if (!CallRemoteQI(iid))
				return 0;

			// Create a new proxy for this interface
			ptrProxy.Attach(OOCore::CreateProxy(iid,this));
		}

		OOBase::Guard<OOBase::RWMutex> guard(m_lock);
			
		std::pair<std::map<const guid_t,ObjectPtr<IObject> >::iterator,bool> p=m_iid_map.insert(std::map<const guid_t,ObjectPtr<IObject> >::value_type(iid,ptrProxy));
		if (!p.second)
			ptrProxy = p.first->second;
	
		return ptrProxy.AddRef();
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Remoting::IMessage* OOCore::Proxy::CallRemoteStubMarshal(Remoting::IObjectManager* pObjectManager, const guid_t& iid)
{
	ObjectPtr<Remoting::IMessage> pParamsOut;
	pParamsOut.Attach(m_pManager->CreateMessage());

	WriteStubInfo(pParamsOut,2);

	System::MetaInfo::wire_write(L"iid",pParamsOut,iid);

	Remoting::IMessage* pParamsIn = 0;
	IException* pE = 0;

	try
	{
		m_pManager->DoMarshalChannel(pObjectManager,pParamsOut);

		pParamsOut->WriteStructEnd(L"ipc_request");
	
		pE = m_pManager->SendAndReceive(TypeInfo::Synchronous,pParamsOut,pParamsIn);
	}
	catch (...)
	{
		ReadStubInfo(pParamsOut);

		guid_t m;
		System::MetaInfo::wire_read(L"iid",pParamsOut,m);

		void* TODO; // Release marshal data for channel
		//m_pManager->ReleaseMarshalData(L"pObjectManager",pParamsOut,OMEGA_GUIDOF(System::IMarshaller),pObjectManager);

		throw;
	}

	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	if (pE)
		throw pE;

	IObject* pUI = 0;
	m_pManager->UnmarshalInterface(L"pReflect",ptrParamsIn,OMEGA_GUIDOF(Remoting::IMessage),pUI);
	return static_cast<Remoting::IMessage*>(pUI);
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

		System::MetaInfo::wire_write(L"release_count",pParamsOut,static_cast<Omega::uint32_t>(m_marshal_count.value()));

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

void OOCore::Proxy::MarshalInterface(Remoting::IObjectManager* pObjectManager, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t)
{
	// Tell the stub to expect incoming requests from a different channel...
	ObjectPtr<Remoting::IMessage> ptrReflect;
	ptrReflect.Attach(CallRemoteStubMarshal(pObjectManager,iid));
	
	return pObjectManager->MarshalInterface(L"pReflect",pMessage,OMEGA_GUIDOF(Remoting::IMessage),ptrReflect);
}

void OOCore::Proxy::ReleaseMarshalData(Remoting::IObjectManager*, Remoting::IMessage*, const guid_t&, Remoting::MarshalFlags_t)
{
	// How do we undo this?
	void* TODO;
}

void OOCore::ProxyMarshalFactory::UnmarshalInterface(Remoting::IObjectManager* pObjectManager, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	// Unmarshal the reflect package
	IObject* pUI = 0;
	pObjectManager->UnmarshalInterface(L"pReflect",pMessage,OMEGA_GUIDOF(Remoting::IMessage),pUI);
	ObjectPtr<Remoting::IMessage> ptrReflect;
	ptrReflect.Attach(static_cast<Remoting::IMessage*>(pUI));

	// Unmarshal the manager
	pUI = 0;
	pObjectManager->UnmarshalInterface(L"m_ptrChannel",ptrReflect,OMEGA_GUIDOF(Remoting::IChannel),pUI);
	ObjectPtr<Remoting::IChannel> ptrChannel;
	ptrChannel.Attach(static_cast<Remoting::IChannel*>(pUI));

	ObjectPtr<Remoting::IObjectManager> ptrOM;
	ptrOM.Attach(ptrChannel->GetObjectManager());

	// Unmarshal the new proxy on the new manager
	ptrOM->UnmarshalInterface(L"stub",ptrReflect,iid,pObject);
}
