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
		m_marshal_count(0),
		m_stub_id(0),
		m_pManager(0)
{}

void OOCore::Stub::init(IObject* pObj, uint32_t stub_id, StdObjectManager* pManager)
{
	m_stub_id = stub_id;
	m_ptrObj = pObj;
	m_ptrObj.AddRef();
	m_pManager = pManager;
}

void OOCore::Stub::MarshalInterface(Remoting::IMessage* pMessage, const guid_t& iid)
{
	pMessage->WriteValue(string_t::constant("id"),m_stub_id);
	pMessage->WriteValue(string_t::constant("iid"),iid);

	++m_marshal_count;
}

void OOCore::Stub::ReleaseMarshalData(Remoting::IMessage* pMessage, const guid_t&)
{
	// Deref safely
	RemoteRelease();

	pMessage->ReadValue(string_t::constant("id"));
	pMessage->ReadValue(string_t::constant("iid"));
}

void OOCore::Stub::Invoke(Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
{
	ObjectPtr<Remoting::IStub> ptrStub = FindStub(pParamsIn->ReadValue(string_t::constant("$iid")).cast<guid_t>());
	if (!ptrStub)
		OMEGA_THROW("Invoke on unsupported interface");

	ptrStub->Invoke(pParamsIn,pParamsOut);
}

Remoting::IStub* OOCore::Stub::FindStub(const guid_t& iid)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// See if we have a stub for this interface already...
	ObjectPtr<Remoting::IStub> ptrStub;
	if (!m_iid_map.find(iid,ptrStub))
	{
		// See if any known interface supports the new interface
		for (size_t i=m_iid_map.begin(); i!=m_iid_map.npos; i=m_iid_map.next(i))
		{
			ObjectPtr<Remoting::IStub> ptrStub2 = *m_iid_map.at(i);
			if (ptrStub2 && ptrStub2->SupportsInterface(iid))
			{
				ptrStub = ptrStub2;
				break;
			}
		}

		if (!ptrStub)
			ptrStub = CreateStub(iid);

		// Now add it...
		int err = m_iid_map.insert(iid,ptrStub);
		if (err != 0)
			OMEGA_THROW(err);
	}

	return ptrStub.Detach();
}

Remoting::IStub* OOCore::Stub::CreateStub(const guid_t& iid)
{
	ObjectPtr<System::Internal::ISafeProxy> ptrSafeProxy = m_ptrObj.QueryInterface<System::Internal::ISafeProxy>();
	if (ptrSafeProxy)
	{
		// Create the stubs for the controllers
		System::Internal::auto_safe_shim shim_Controller = System::Internal::create_safe_stub(static_cast<Remoting::IStubController*>(this),OMEGA_GUIDOF(Remoting::IStubController));
		System::Internal::auto_safe_shim shim_MarshalContext = System::Internal::create_safe_stub(static_cast<Remoting::IMarshalContext*>(m_pManager),OMEGA_GUIDOF(Remoting::IMarshalContext));

		System::Internal::auto_safe_shim wire_stub = ptrSafeProxy->CreateWireStub(shim_Controller,shim_MarshalContext,iid);

		return System::Internal::create_safe_proxy<Remoting::IStub>(wire_stub);
	}

	return System::Internal::create_wire_stub(this,m_pManager,iid,m_ptrObj);
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
	ObjectPtr<Remoting::IStub> ptrStub = FindStub(iid);
	return (ptrStub != NULL);
}

void OOCore::Stub::MarshalStub(Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
{
	guid_t iid = pParamsIn->ReadValue(string_t::constant("iid")).cast<guid_t>();

	// Unmarshal the channel
	ObjectPtr<Remoting::IChannel> ptrChannel;
	ptrChannel.Unmarshal(m_pManager,string_t::constant("m_ptrChannel"),pParamsIn);
	if (!ptrChannel)
		OMEGA_THROW("No channel");

	// Create a new message
	ObjectPtr<Remoting::IMessage> ptrMessage = ptrChannel->CreateMessage();

	// Reflect the channel
	// The following format is the same as IObjectManager::UnmarshalInterface...
	ptrMessage->WriteStructStart(string_t::constant("m_ptrChannel"),string_t::constant("$iface_marshal"));
	ptrMessage->WriteValue(string_t::constant("$marshal_type"),byte_t(2));
	ptrMessage->WriteValue(string_t::constant("$oid"),ptrChannel->GetReflectUnmarshalFactoryOID());

	ptrChannel->ReflectMarshal(ptrMessage);

	ptrMessage->WriteStructEnd();

	// Get the channel marshal flags
	Remoting::MarshalFlags_t flags = ptrChannel->GetMarshalFlags();

	// Get the channel's IMarshalContext
	IObject* pObj = NULL;
	ptrChannel->GetManager(OMEGA_GUIDOF(Remoting::IMarshalContext),pObj);
	ObjectPtr<Remoting::IMarshalContext> ptrMarshalContext = static_cast<Remoting::IMarshalContext*>(pObj);
	if (!ptrMarshalContext)
		throw OOCore_INotFoundException_MissingIID(OMEGA_GUIDOF(Remoting::IMarshalContext));

	// QI for iid
	ObjectPtr<IObject> ptrObject = m_ptrObj->QueryInterface(iid);
	if (!ptrObject)
		throw OOCore_INotFoundException_MissingIID(iid);

	// Marshal the stub
	ptrMarshalContext->MarshalInterface(string_t::constant("stub"),ptrMessage,iid,ptrObject);

	try
	{
		m_pManager->MarshalInterface(string_t::constant("pReflect"),pParamsOut,OMEGA_GUIDOF(Remoting::IMessage),ptrMessage);

		if (flags != Remoting::Same)
			++m_marshal_count;
	}
	catch (...)
	{
		ptrMarshalContext->ReleaseMarshalData(string_t::constant("stub"),ptrMessage,iid,m_ptrObj);
		throw;
	}
}
