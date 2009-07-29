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
	//ObjectPtr<System::IStub> ptrStub = FindStub(iid);
	
	System::MetaInfo::wire_write(L"id",pMessage,m_stub_id);
	try
	{
		System::MetaInfo::wire_write(L"iid",pMessage,iid);
	}
	catch (...)
	{
		uint32_t v;
		System::MetaInfo::wire_read(L"id",pMessage,v);
		throw;
	}

	++m_marshal_count;
}

void OOCore::Stub::ReleaseMarshalData(Remoting::IMessage* pMessage, const guid_t&)
{
	// Deref safely
	RemoteRelease(1);

	uint32_t v;
	System::MetaInfo::wire_read(L"id",pMessage,v);
		
	guid_t iid;
	System::MetaInfo::wire_read(L"iid",pMessage,iid);
}

void OOCore::Stub::Invoke(Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
{
	// Read the method id
	uint32_t method_id = 0;
	System::MetaInfo::wire_read(L"$method_id",pParamsIn,method_id);

	switch (method_id)
	{
	case 0: // RemoteRelease
		{
			uint32_t release_count = 0;
			System::MetaInfo::wire_read(L"release_count",pParamsIn,release_count);
			RemoteRelease(release_count);
		}
		break;

	case 1: // QueryInterface
		{
			guid_t iid;
			System::MetaInfo::wire_read(L"iid",pParamsIn,iid);
			
			bool_t bQI = RemoteQueryInterface(iid);
			System::MetaInfo::wire_write(L"bQI",pParamsOut,bQI);
		}
		break;

	case 2: // MarshalStub
		MarshalStub(pParamsIn,pParamsOut);
		break;

	default:
		OMEGA_THROW(L"Invoke called with invalid method index");
	}
}

ObjectPtr<System::IStub> OOCore::Stub::LookupStub(Remoting::IMessage* pMessage)
{
	guid_t iid;
	System::MetaInfo::wire_read(L"$iid",pMessage,iid);
	if (iid == OMEGA_GUIDOF(IObject))
		return static_cast<IStub*>(this);

	return FindStub(iid);
}

ObjectPtr<System::IStub> OOCore::Stub::FindStub(const guid_t& iid)
{
	try
	{
		OOBase::Guard<OOBase::SpinLock> guard(m_lock);

		ObjectPtr<System::IStub> ptrStub;
			
		// See if we have a stub for this interface already...
		std::map<const guid_t,ObjectPtr<System::IStub> >::iterator i=m_iid_map.find(iid);
		if (i != m_iid_map.end())
			return i->second;
			
		// See if any known interface supports the new interface
		for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
		{
			if (i->second->SupportsInterface(iid))
			{
				ptrStub = i->second;
				break;
			}
		}
	
		if (!ptrStub)
		{
			// Check whether underlying object supports interface
			IObject* pQI = m_ptrObj->QueryInterface(iid);
			if (!pQI)
				return 0;

			ObjectPtr<IObject> ptrQI;
			ptrQI.Attach(pQI);

			// Create a stub for this interface
			/*ptrStub.Attach(OOCore::CreateStub(iid,this,m_pManager,pQI));
			if (!ptrStub)*/
				throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
		}

		// Now add it...
		std::pair<std::map<const guid_t,ObjectPtr<System::IStub> >::iterator,bool> p=m_iid_map.insert(std::map<const guid_t,ObjectPtr<System::IStub> >::value_type(iid,ptrStub));
		if (!p.second)
			ptrStub = p.first->second;
				
		return ptrStub;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void OOCore::Stub::RemoteRelease(uint32_t release_count)
{
	m_marshal_count -= release_count;
	if (m_marshal_count == 0)
		m_pManager->RemoveStub(m_stub_id);
}

bool_t OOCore::Stub::RemoteQueryInterface(const guid_t& iid)
{
	// If we have a stub, then we can handle it...
	return (FindStub(iid) != 0);
}

void OOCore::Stub::MarshalStub(Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
{
	guid_t iid;
	System::MetaInfo::wire_read(L"iid",pParamsIn,iid);
	
	// Unmarshal the channel
	IObject* pUI = 0;
	m_pManager->UnmarshalInterface(L"m_ptrChannel",pParamsIn,OMEGA_GUIDOF(Remoting::IChannel),pUI);
	ObjectPtr<Remoting::IChannel> ptrChannel;
	ptrChannel.Attach(static_cast<Remoting::IChannel*>(pUI));

	// Create a new message
	ObjectPtr<Remoting::IMessage> ptrMessage;
	ptrMessage.Attach(ptrChannel->CreateMessage());
	
	// Reflect the channel
	// The following format is the same as IObjectManager::UnmarshalInterface...
	ptrMessage->WriteStructStart(L"m_ptrChannel",L"$iface_marshal");
	System::MetaInfo::wire_write(L"$marshal_type",ptrMessage,(byte_t)2);
	System::MetaInfo::wire_write(L"$oid",ptrMessage,ptrChannel->GetReflectUnmarshalFactoryOID());
	
	ptrChannel->ReflectMarshal(ptrMessage);
	
	ptrMessage->WriteStructEnd(L"m_ptrChannel");
	
	// Get the channel's OM
	ObjectPtr<Remoting::IObjectManager> ptrOM;
	ptrOM.Attach(ptrChannel->GetObjectManager());
	
	// Marshal the stub
	ptrOM->MarshalInterface(L"stub",ptrMessage,iid,m_ptrObj);
	
	try
	{
		m_pManager->MarshalInterface(L"pReflect",pParamsOut,OMEGA_GUIDOF(Remoting::IMessage),ptrMessage);
	}
	catch (...)
	{
		ptrOM->ReleaseMarshalData(L"stub",ptrMessage,iid,m_ptrObj);
		throw;
	}
}
