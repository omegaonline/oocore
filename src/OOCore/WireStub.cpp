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

#include "WireStub.h"
#include "StdObjectManager.h"

using namespace Omega;
using namespace OTL;

OOCore::Stub::Stub() : 
	m_stub_id(0), m_pManager(0)
{
}

void OOCore::Stub::init(IObject* pObj, uint32_t stub_id, StdObjectManager* pManager)
{
	m_stub_id = stub_id;
	m_ptrObj = pObj;
	m_pManager = pManager;
}

void OOCore::Stub::MarshalInterface(Remoting::IMessage* pMessage, const guid_t& iid)
{
	// Make sure we can support the outgoing interface...
	assert(FindStub(iid) != 0);
	
	pMessage->WriteUInt32(L"id",m_stub_id);
	pMessage->WriteGuid(L"iid",iid);
	
	++m_marshal_count;
}

void OOCore::Stub::ReleaseMarshalData(Remoting::IMessage* pMessage, const guid_t&)
{
	// Deref safely
	RemoteRelease();

	pMessage->ReadUInt32(L"id");
	pMessage->ReadGuid(L"iid");
}

void OOCore::Stub::Invoke(Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
{
	ObjectPtr<Remoting::IStub> ptrStub = FindStub(pParamsIn->ReadGuid(L"$iid"));
	if (!ptrStub)
		OMEGA_THROW(L"Invoke on unsupported interface");

	ptrStub->Invoke(pParamsIn,pParamsOut);
}

ObjectPtr<Remoting::IStub> OOCore::Stub::FindStub(const guid_t& iid)
{
	try
	{
		OOBase::Guard<OOBase::SpinLock> guard(m_lock);

		// See if we have a stub for this interface already...
		std::map<const guid_t,ObjectPtr<Remoting::IStub> >::iterator i=m_iid_map.find(iid);
		if (i != m_iid_map.end())
			return i->second;
			
		// See if any known interface supports the new interface
		ObjectPtr<Remoting::IStub> ptrStub;
		for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
		{
			if (i->second && i->second->SupportsInterface(iid))
			{
				ptrStub = i->second;
				break;
			}
		}
	
		if (!ptrStub)
			ptrStub.Attach(CreateStub(iid));
			
		// Now add it...
		std::pair<std::map<const guid_t,ObjectPtr<Remoting::IStub> >::iterator,bool> p=m_iid_map.insert(std::map<const guid_t,ObjectPtr<Remoting::IStub> >::value_type(iid,ptrStub));
		if (!p.second)
			ptrStub = p.first->second;
				
		return ptrStub;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Remoting::IStub* OOCore::Stub::CreateStub(const guid_t& iid)
{
	ObjectPtr<System::MetaInfo::ISafeProxy> ptrSafeProxy(m_ptrObj);
	if (ptrSafeProxy)
	{
		// Create the stubs for the controllers
		System::MetaInfo::auto_safe_shim shim_Controller = System::MetaInfo::create_safe_stub(static_cast<Remoting::IStubController*>(this),OMEGA_GUIDOF(Remoting::IStubController));
		System::MetaInfo::auto_safe_shim shim_Marshaller = System::MetaInfo::create_safe_stub(static_cast<Remoting::IMarshaller*>(m_pManager),OMEGA_GUIDOF(Remoting::IMarshaller));

		System::MetaInfo::auto_safe_shim wire_stub = ptrSafeProxy->CreateWireStub(shim_Controller,shim_Marshaller,iid);

		return System::MetaInfo::create_safe_proxy<Remoting::IStub>(wire_stub);
	}
	
	return System::MetaInfo::create_wire_stub(this,m_pManager,iid,m_ptrObj);	
}

void OOCore::Stub::RemoteRelease()
{
	if (--m_marshal_count == 0)
	{
		// This will Release() us...
		m_pManager->RemoveStub(m_stub_id);
	}
}

bool_t OOCore::Stub::RemoteQueryInterface(const guid_t& iid)
{
	// If we have a stub, then we can handle it...
	return (FindStub(iid) != 0);
}

void OOCore::Stub::MarshalStub(Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
{
	guid_t iid = pParamsIn->ReadGuid(L"iid");
	
	// Unmarshal the channel
	ObjectPtr<Remoting::IChannel> ptrChannel = ObjectPtr<Remoting::IMarshaller>(static_cast<Remoting::IMarshaller*>(m_pManager)).UnmarshalInterface<Remoting::IChannel>(L"m_ptrChannel",pParamsIn);
	if (!ptrChannel)
		OMEGA_THROW(L"No channel");
	
	// Create a new message
	ObjectPtr<Remoting::IMessage> ptrMessage;
	ptrMessage.Attach(ptrChannel->CreateMessage());
	
	// Reflect the channel
	// The following format is the same as IObjectManager::UnmarshalInterface...
	ptrMessage->WriteStructStart(L"m_ptrChannel",L"$iface_marshal");
	ptrMessage->WriteByte(L"$marshal_type",2);
	ptrMessage->WriteGuid(L"$oid",ptrChannel->GetReflectUnmarshalFactoryOID());
	
	ptrChannel->ReflectMarshal(ptrMessage);
	
	ptrMessage->WriteStructEnd(L"m_ptrChannel");
	
	// Get the channel's IMarshaller
	ObjectPtr<Remoting::IMarshaller> ptrMarshaller = ptrChannel.GetManager<Remoting::IMarshaller>();
	if (!ptrMarshaller)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller),OMEGA_SOURCE_INFO);
	
	// Marshal the stub
	ptrMarshaller->MarshalInterface(L"stub",ptrMessage,iid,m_ptrObj);
	
	try
	{
		m_pManager->MarshalInterface(L"pReflect",pParamsOut,OMEGA_GUIDOF(Remoting::IMessage),ptrMessage);
	}
	catch (...)
	{
		ptrMarshaller->ReleaseMarshalData(L"stub",ptrMessage,iid,m_ptrObj);
		throw;
	}
}
