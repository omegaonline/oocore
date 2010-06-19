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
		m_ptrOF.Attach(Activation::GetObjectFactory(m_message_oid,Activation::InProcess));
	
	// Connect the OM to us
	m_ptrOM = pOM;
	if (m_ptrOM)
		m_ptrOM->Connect(this);
}

void OOCore::ChannelBase::disconnect()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	ObjectPtr<Remoting::IObjectManager> ptrOM = m_ptrOM;
	m_ptrOM.Release();

	guard.release();

	if (ptrOM)
		ptrOM->Shutdown();
}

ObjectPtr<Remoting::IObjectManager> OOCore::ChannelBase::GetObjectManager()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	if (!m_ptrOM)
		throw Remoting::IChannelClosedException::Create();

	return m_ptrOM;
}

Remoting::IMessage* OOCore::ChannelBase::CreateMessage()
{
	if (m_message_oid == guid_t::Null())
	{
		// Create a fresh CDRMessage
		return static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::CDRMessage>::CreateInstance());
	}
	else
	{
		// Create a fresh one
		IObject* pObject = 0;
		m_ptrOF->CreateInstance(0,OMEGA_GUIDOF(Remoting::IMessage),pObject);
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
		throw Remoting::IChannelClosedException::Create();

	pObject = m_ptrOM->QueryInterface(iid);
}

guid_t OOCore::ChannelBase::GetUnmarshalFactoryOID(const guid_t&, Remoting::MarshalFlags_t)
{
	return OID_ChannelMarshalFactory;
}

void OOCore::ChannelBase::MarshalInterface(Remoting::IMarshaller*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	pMessage->WriteValue(L"m_channel_id",m_channel_id);
	pMessage->WriteValue(L"m_message_oid",m_message_oid);
}

void OOCore::ChannelBase::ReleaseMarshalData(Remoting::IMarshaller*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	pMessage->ReadValue(L"m_channel_id");
	pMessage->ReadValue(L"m_message_oid");
}

void OOCore::ChannelBase::ReflectMarshal(Remoting::IMessage* pMessage)
{
	MarshalInterface(0,pMessage,guid_t::Null(),m_marshal_flags);
}

void OOCore::Channel::init(UserSession* pSession, uint32_t channel_id, Remoting::IObjectManager* pOM, const guid_t& message_oid)
{
	ChannelBase::init(channel_id,pSession->classify_channel(channel_id),pOM,message_oid);

	m_pSession = pSession;
	
	// QI for IMarshaller
	m_ptrMarshaller = m_ptrOM;
	if (!m_ptrMarshaller)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller));
}

void OOCore::Channel::disconnect()
{
	ChannelBase::disconnect();

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	m_ptrMarshaller.Release();
}

IException* OOCore::Channel::SendAndReceive(TypeInfo::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv,  uint32_t timeout)
{
	// Get the object manager
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	ObjectPtr<Remoting::IMarshaller> ptrMarshaller = m_ptrMarshaller;
	
	guard.release();

	if (!ptrMarshaller)
		throw Remoting::IChannelClosedException::Create();

	// We need to wrap the message
	ObjectPtr<ObjectImpl<CDRMessage> > ptrEnvelope = ObjectImpl<CDRMessage>::CreateInstancePtr();
	ptrMarshaller->MarshalInterface(L"payload",ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pSend);

	OOBase::SmartPtr<OOBase::CDRStream> response = 0;
	try
	{
		response = m_pSession->send_request(m_channel_id,ptrEnvelope->GetCDRStream(),timeout,attribs);
	}
	catch (Remoting::IChannelClosedException* pE)
	{
		ptrMarshaller->ReleaseMarshalData(L"payload",ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pSend);
		disconnect();
		pE->Rethrow();
	}
	catch (...)
	{
		ptrMarshaller->ReleaseMarshalData(L"payload",ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pSend);
		throw;
	}

	if (response)
	{
		try
		{

			// Wrap the response
			ObjectPtr<ObjectImpl<CDRMessage> > ptrRecv = ObjectImpl<CDRMessage>::CreateInstancePtr();
			ptrRecv->init(*response);

			// Unwrap the payload...
			pRecv = ptrMarshaller.UnmarshalInterface<Remoting::IMessage>(L"payload",ptrRecv).AddRef();
		}
		catch (IException* pE)
		{
			return pE;
		}
	}
	return 0;
}

bool_t OOCore::Channel::IsConnected()
{
	OOBase::SmartPtr<OOBase::CDRStream> response = 0;

	bool connected = true;
	try
	{
		response = m_pSession->send_request(m_channel_id,0,0,Message::synchronous | Message::channel_ping);
	}
	catch (Remoting::IChannelClosedException* pE)
	{
		pE->Release();
		connected = false;
	}
		
	if (connected)
	{
		byte_t pong = 0;
		if (!response || !response->read(pong))
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

void OOCore::Channel::ReflectMarshal(Remoting::IMessage* pMessage)
{
	OOBase::SmartPtr<OOBase::CDRStream> response = 0;

	try
	{
		response = m_pSession->send_request(m_channel_id,0,0,Message::synchronous | Message::channel_reflect);
	}
	catch (Remoting::IChannelClosedException*)
	{
		// Disconnect ourselves on failure
		disconnect();
		throw;
	}

	uint32_t other_end = 0;
	if (!response || !response->read(other_end))
		OMEGA_THROW("Unexpected end of message");

	// Return in the same format as we marshal
	pMessage->WriteValue(L"m_channel_id",other_end);
	pMessage->WriteValue(L"m_message_oid",m_message_oid);
}

OMEGA_DEFINE_OID(OOCore,OID_ChannelMarshalFactory,"{7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}");

void OOCore::ChannelMarshalFactory::UnmarshalInterface(Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t flags, IObject*& pObject)
{
	if (OOCore::HostedByOOServer())
	{
		// This must match OOServer::User::OID_ChannelMarshalFactory
		static const guid_t oid(L"{1A7672C5-8478-4e5a-9D8B-D5D019E25D15}");
		ObjectPtr<Remoting::IMarshalFactory> ptrMarshalFactory(oid,Activation::InProcess | Activation::DontLaunch);

		// If we have a pointer by now then we are actually running in the OOServer.exe,
		// and can therefore do our specialized unmarshalling...
		ptrMarshalFactory->UnmarshalInterface(pMarshaller,pMessage,iid,flags,pObject);
	}
	else
	{
		// If we get here, then we are loaded into a different exe from OOServer,
		// therefore we do simple unmarshalling

		uint32_t channel_id = pMessage->ReadValue(L"m_channel_id").cast<uint32_t>();
		guid_t message_oid = pMessage->ReadValue(L"m_message_oid").cast<guid_t>();

		// Create a new channel
		pObject = UserSession::create_channel(channel_id,message_oid,iid);
	}
}

OMEGA_DEFINE_OID(OOCore,OID_CDRMessageMarshalFactory,"{1455FCD0-A49B-4f2a-94A5-222949957123}");

void OOCore::CDRMessageMarshalFactory::UnmarshalInterface(Remoting::IMarshaller*, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	uint32_t len = pMessage->ReadValue(L"length").cast<uint32_t>();

	OOBase::CDRStream input(len);
	pMessage->ReadBytes(L"data",len,(byte_t*)input.buffer()->wr_ptr());
	input.buffer()->wr_ptr(len);

	ObjectPtr<ObjectImpl<CDRMessage> > ptrInput = ObjectImpl<CDRMessage>::CreateInstancePtr();
	ptrInput->init(input);

	pObject = ptrInput->QueryInterface(iid);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::Remoting::IMessage*,OOCore_Remoting_CreateMemoryMessage,0,())
{
	return static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::CDRMessage>::CreateInstance());
}
