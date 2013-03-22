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

#include "Channel.h"
#include "UserSession.h"

using namespace Omega;
using namespace OTL;

OOCore::ChannelBase::ChannelBase() :
		m_channel_id(0),
		m_marshal_flags(0)
{
}

void OOCore::ChannelBase::init(uint32_t channel_id, Remoting::MarshalFlags_t marshal_flags, Remoting::IObjectManager* pOM, const guid_t& message_oid)
{
	m_channel_id = channel_id;
	m_marshal_flags = marshal_flags;
	m_message_oid = message_oid;

	if (m_message_oid != guid_t::Null())
		m_ptrOF.GetObject(m_message_oid,Activation::Library);

	// Connect the OM to us
	m_ptrOM = pOM;
	m_ptrOM.AddRef();
	if (m_ptrOM)
		m_ptrOM->Connect(this);
}

void OOCore::ChannelBase::on_disconnect()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	ObjectPtr<Remoting::IObjectManager> ptrOM = m_ptrOM;
	m_ptrOM.Release();

	guard.release();

	if (ptrOM)
		ptrOM->Shutdown();
}

Remoting::IObjectManager* OOCore::ChannelBase::GetObjectManager()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	if (!m_ptrOM)
		throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("GetObjectManager() called on disconnected channel"));

	return m_ptrOM.AddRef();
}

Remoting::IMessage* OOCore::ChannelBase::CreateMessage()
{
	if (m_message_oid == guid_t::Null())
	{
		// Create a fresh CDRMessage
		return ObjectImpl<OOCore::CDRMessage>::CreateObject();
	}
	else
	{
		// Create a fresh one
		IObject* pObject = NULL;
		m_ptrOF->CreateObject(OMEGA_GUIDOF(Remoting::IMessage),pObject);
		return static_cast<Remoting::IMessage*>(pObject);
	}
}

Remoting::MarshalFlags_t OOCore::ChannelBase::GetMarshalFlags()
{
	return m_marshal_flags;
}

uint32_t OOCore::ChannelBase::GetSource()
{
	return m_channel_id;
}

guid_t OOCore::ChannelBase::GetReflectUnmarshalFactoryOID()
{
	return OID_ChannelMarshalFactory;
}

void OOCore::ChannelBase::GetManager(const guid_t& iid, IObject*& pObject)
{
	// Get the object manager
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	if (!m_ptrOM)
		throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("GetManager() called on disconnected channel"));

	pObject = m_ptrOM->QueryInterface(iid);
}

guid_t OOCore::ChannelBase::GetUnmarshalFactoryOID(const guid_t&, Remoting::MarshalFlags_t)
{
	return OID_ChannelMarshalFactory;
}

void OOCore::ChannelBase::MarshalInterface(Remoting::IMarshalContext*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	pMessage->WriteValue(string_t::constant("m_channel_id"),m_channel_id);
	pMessage->WriteValue(string_t::constant("m_message_oid"),m_message_oid);
}

void OOCore::ChannelBase::ReleaseMarshalData(Remoting::IMarshalContext*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	pMessage->ReadValue(string_t::constant("m_channel_id"));
	pMessage->ReadValue(string_t::constant("m_message_oid"));
}

void OOCore::ChannelBase::ReflectMarshal(Remoting::IMessage* pMessage)
{
	MarshalInterface(NULL,pMessage,guid_t::Null(),m_marshal_flags);
}

void OOCore::Channel::init(UserSession* pSession, Remoting::MarshalFlags_t flags, uint32_t channel_id, Remoting::IObjectManager* pOM, const guid_t& message_oid)
{
	ChannelBase::init(channel_id,flags,pOM,message_oid);

	m_pSession = pSession;
	
	// QI for IMarshalContext
	m_ptrMarshalContext = m_ptrOM.QueryInterface<Remoting::IMarshalContext>();
	if (!m_ptrMarshalContext)
		throw OOCore_INotFoundException_MissingIID(OMEGA_GUIDOF(Remoting::IMarshalContext));
}

void OOCore::Channel::on_disconnect()
{
	ChannelBase::on_disconnect();

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	m_ptrMarshalContext.Release();
}

void OOCore::Channel::shutdown(uint32_t closed_channel_id)
{
	// Send a channel close back to the sender
	OOBase::CDRStream msg;
	if (msg.write(closed_channel_id))
		m_pSession->send_request(m_channel_id,&msg,NULL,Message::asynchronous | Message::channel_close);

	on_disconnect();
}

IException* OOCore::Channel::SendAndReceive(TypeInfo::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv)
{
	// Get the object manager
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// The OM is actually the 'controlling' object for open state
	if (!m_ptrOM)
		throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("SendAndReceive() called on disconnected channel"));

	ObjectPtr<Remoting::IMarshalContext> ptrMarshalContext = m_ptrMarshalContext;
	
	guard.release();

	// We need to wrap the message
	ObjectPtr<ObjectImpl<CDRMessage> > ptrEnvelope = ObjectImpl<CDRMessage>::CreateObject();
	ptrMarshalContext->MarshalInterface(string_t::constant("payload"),ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pSend);

	OOBase::CDRStream response;
	try
	{
		m_pSession->send_request(m_channel_id,ptrEnvelope->GetCDRStream(),&response,attribs);
	}
	catch (Remoting::IChannelClosedException* pE)
	{
		ptrMarshalContext->ReleaseMarshalData(string_t::constant("payload"),ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pSend);
		on_disconnect();
		pE->Rethrow();
	}
	catch (...)
	{
		ptrMarshalContext->ReleaseMarshalData(string_t::constant("payload"),ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pSend);
		throw;
	}

	if (!(attribs & TypeInfo::Asynchronous))
	{
		try
		{
			// Wrap the response
			ObjectPtr<ObjectImpl<CDRMessage> > ptrRecv = ObjectImpl<CDRMessage>::CreateObject();
			ptrRecv->init(response);

			// Unwrap the payload...
			IObject* pObj = NULL;
			ptrMarshalContext->UnmarshalInterface(string_t::constant("payload"),ptrRecv,OMEGA_GUIDOF(Remoting::IMessage),pObj);
			pRecv = static_cast<Remoting::IMessage*>(pObj);
		}
		catch (IException* pE)
		{
			return pE;
		}
	}
	return NULL;
}

bool_t OOCore::Channel::IsConnected()
{
	OOBase::CDRStream response;
	bool connected = true;

	try
	{
		m_pSession->send_request(m_channel_id,NULL,&response,Message::synchronous | Message::channel_ping);
	}
	catch (Remoting::IChannelClosedException* pE)
	{
		pE->Release();
		connected = false;
	}
		
	if (connected)
	{
		byte_t pong = 0;
		if (!response.read(pong))
			connected = false;
		else if (pong != 1)
			connected = false;
	}

	if (!connected)
	{
		// Disconnect ourselves
		on_disconnect();
	}

	return connected;
}

void OOCore::Channel::ReflectMarshal(Remoting::IMessage* pMessage)
{
	OOBase::CDRStream response;
	try
	{
		m_pSession->send_request(m_channel_id,NULL,&response,Message::synchronous | Message::channel_reflect);
	}
	catch (Remoting::IChannelClosedException* pE)
	{
		// Disconnect ourselves on failure
		on_disconnect();
		pE->Rethrow();
	}

	uint32_t other_end = 0;
	if (!response.read(other_end))
		OMEGA_THROW("Unexpected end of message");

	// Return in the same format as we marshal
	pMessage->WriteValue(string_t::constant("m_channel_id"),other_end);
	pMessage->WriteValue(string_t::constant("m_message_oid"),m_message_oid);
}

const Omega::guid_t OOCore::OID_ChannelMarshalFactory("{7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}");

void OOCore::ChannelMarshalFactory::UnmarshalInterface(Remoting::IMarshalContext* pMarshalContext, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t flags, IObject*& pObject)
{
	uint32_t channel_id = pMessage->ReadValue(string_t::constant("m_channel_id")).cast<uint32_t>();
	guid_t message_oid = pMessage->ReadValue(string_t::constant("m_message_oid")).cast<guid_t>();

	// Create a new channel
	pObject = UserSession::create_channel(channel_id,message_oid,iid,flags);
}

const Omega::guid_t OOCore::OID_CDRMessageMarshalFactory("{1455FCD0-A49B-4f2a-94A5-222949957123}");

void OOCore::CDRMessageMarshalFactory::UnmarshalInterface(Remoting::IMarshalContext*, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	uint32_t len = pMessage->ReadValue(string_t::constant("length")).cast<uint32_t>();

	OOBase::CDRStream input(len);
	pMessage->ReadBytes(string_t::constant("data"),len,(byte_t*)input.buffer()->wr_ptr());
	input.buffer()->wr_ptr(len);

	ObjectPtr<ObjectImpl<CDRMessage> > ptrInput = ObjectImpl<CDRMessage>::CreateObject();
	ptrInput->init(input);

	pObject = ptrInput->QueryInterface(iid);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Remoting::IMessage*,OOCore_Remoting_CreateMemoryMessage,0,())
{
	return ObjectImpl<OOCore::CDRMessage>::CreateObject();
}
