///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer_User.h"
#include "UserChannel.h"
#include "UserManager.h"

using namespace Omega;
using namespace OTL;

const Omega::guid_t User::OID_ChannelMarshalFactory("{7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}");

User::Channel::Channel() :
		m_channel_id(0),
		m_marshal_flags(0)
{
}

void User::Channel::init(Omega::uint32_t channel_id, Remoting::MarshalFlags_t marshal_flags, const guid_t& message_oid)
{
	m_channel_id = channel_id;
	m_marshal_flags = marshal_flags;
	m_message_oid = message_oid;

	if (m_message_oid != guid_t::Null())
		m_ptrOF.GetObject(m_message_oid,Activation::Library);

	// Create a new OM
	m_ptrOM = OOCore_CreateStdObjectManager();

	// QI for IMarshaller
	m_ptrMarshaller = m_ptrOM.QueryInterface<Remoting::IMarshaller>();
	if (!m_ptrMarshaller)
		throw OOCore_INotFoundException_MissingIID(OMEGA_GUIDOF(Remoting::IMarshaller));

	// Associate it with the channel
	m_ptrOM->Connect(this);
}

void User::Channel::disconnect()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	ObjectPtr<Remoting::IObjectManager> ptrOM = m_ptrOM;
	m_ptrOM.Release();

	m_ptrMarshaller.Release();

	guard.release();

	if (ptrOM)
		ptrOM->Shutdown();
}

Remoting::IObjectManager* User::Channel::GetObjectManager()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	return m_ptrOM.AddRef();
}

Remoting::IMessage* User::Channel::CreateMessage()
{
	if (m_message_oid == guid_t::Null())
	{
		// Create a fresh CDRMessage
		return ObjectImpl<OOCore::CDRMessage>::CreateObject();
	}
	else
	{
		// Create a fresh one
		IObject* pObject = 0;
		m_ptrOF->CreateObject(OMEGA_GUIDOF(Remoting::IMessage),pObject);
		return static_cast<Remoting::IMessage*>(pObject);
	}
}

IException* User::Channel::SendAndReceive(TypeInfo::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv)
{
	// Get the object manager
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	ObjectPtr<Remoting::IMarshaller> ptrMarshaller = m_ptrMarshaller;
	if (!ptrMarshaller)
		throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("SendAndReceive() called on disconnected channel"));

	guard.release();

	// We need to wrap the message
	ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrEnvelope = ObjectImpl<OOCore::CDRMessage>::CreateObject();
	ptrMarshaller->MarshalInterface(string_t::constant("payload"),ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pSend);

	OOBase::CDRStream response;
	try
	{
		OOServer::MessageHandler::io_result::type res = Manager::instance()->send_request(m_channel_id,ptrEnvelope->GetCDRStream(),&response,attribs);
		if (res != OOServer::MessageHandler::io_result::success)
		{
			if (res == OOServer::MessageHandler::io_result::channel_closed)
			{
				disconnect();
				throw Omega::Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("Failed to send request"));
			}
			
			disconnect();
			OMEGA_THROW("Internal server exception");
		}
	}
	catch (...)
	{
		ptrMarshaller->ReleaseMarshalData(string_t::constant("payload"),ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pSend);
		throw;
	}

	if (!(attribs & TypeInfo::Asynchronous))
	{
		try
		{
			// Wrap the response
			ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrRecv = ObjectImpl<OOCore::CDRMessage>::CreateObject();
			ptrRecv->init(response);

			// Unwrap the payload...
			IObject* pObj = NULL;
			ptrMarshaller->UnmarshalInterface(string_t::constant("payload"),ptrRecv,OMEGA_GUIDOF(Remoting::IMessage),pObj);
			pRecv = static_cast<Remoting::IMessage*>(pObj);
		}
		catch (IException* pE)
		{
			return pE;
		}
	}

	return NULL;
}

Remoting::MarshalFlags_t User::Channel::GetMarshalFlags()
{
	return m_marshal_flags;
}

uint32_t User::Channel::GetSource()
{
	return m_channel_id;
}

guid_t User::Channel::GetReflectUnmarshalFactoryOID()
{
	return GetUnmarshalFactoryOID(guid_t::Null(),0);
}

bool_t User::Channel::IsConnected()
{
	bool connected = true;
	
	OOBase::CDRStream response;
	OOServer::MessageHandler::io_result::type res = Manager::instance()->send_request(m_channel_id,NULL,&response,OOServer::Message_t::synchronous | OOServer::Message_t::channel_ping);
	if (res != OOServer::MessageHandler::io_result::success)
		connected = false;

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
		disconnect();
	}

	return connected;
}

void User::Channel::ReflectMarshal(Remoting::IMessage* pMessage)
{
	OOBase::CDRStream response;
	OOServer::MessageHandler::io_result::type res = Manager::instance()->send_request(m_channel_id,NULL,&response,OOServer::Message_t::synchronous | OOServer::Message_t::channel_reflect);
	if (res != OOServer::MessageHandler::io_result::success)
	{
		if (res == OOServer::MessageHandler::io_result::channel_closed)
		{
			disconnect();
			throw Omega::Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("Failed to send channel_reflect request"));
		}
		
		disconnect();
		OMEGA_THROW("Internal server exception");
	}

	Omega::uint32_t other_end = 0;
	if (!response.read(other_end))
		OMEGA_THROW(response.last_error());

	// Return in the same format as we marshal
	pMessage->WriteValue(string_t::constant("m_channel_id"),other_end);
	pMessage->WriteValue(string_t::constant("m_message_oid"),m_message_oid);
}

void User::Channel::GetManager(const guid_t& iid, IObject*& pObject)
{
	// Get the object manager
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	if (!m_ptrOM)
		throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("GetManager() called on a disconnected channel"));

	pObject = m_ptrOM->QueryInterface(iid);
}

guid_t User::Channel::GetUnmarshalFactoryOID(const guid_t&, Remoting::MarshalFlags_t)
{
	// This must match OOCore::OID_ChannelMarshalFactory
	return User::OID_ChannelMarshalFactory;
}

void User::Channel::MarshalInterface(Remoting::IMarshaller*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	pMessage->WriteValue(string_t::constant("m_channel_id"),m_channel_id);
	pMessage->WriteValue(string_t::constant("m_message_oid"),m_message_oid);
}

void User::Channel::ReleaseMarshalData(Remoting::IMarshaller*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	pMessage->ReadValue(string_t::constant("m_channel_id"));
	pMessage->ReadValue(string_t::constant("m_message_oid"));
}

void User::ChannelMarshalFactory::UnmarshalInterface(Remoting::IMarshaller*, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	uint32_t channel_id = pMessage->ReadValue(string_t::constant("m_channel_id")).cast<uint32_t>();
	guid_t message_oid = pMessage->ReadValue(string_t::constant("m_message_oid")).cast<guid_t>();

	// Create a new object manager (and channel)
	pObject = Manager::create_channel(channel_id,message_oid)->QueryInterface(iid);
}

const Omega::guid_t OOCore::OID_CDRMessageMarshalFactory("{1455FCD0-A49B-4f2a-94A5-222949957123}");
