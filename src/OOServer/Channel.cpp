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
#include "Channel.h"
#include "UserManager.h"

using namespace Omega;
using namespace OTL;

OMEGA_DEFINE_OID(User,OID_ChannelMarshalFactory,"{1A7672C5-8478-4e5a-9D8B-D5D019E25D15}");

User::Channel::Channel() :
	m_pManager(0),
	m_channel_id(0),
	m_marshal_flags(0)
{
}

void User::Channel::init(Manager* pManager, Omega::uint32_t channel_id, Remoting::MarshalFlags_t marshal_flags, const guid_t& message_oid)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	m_pManager = pManager;
	m_channel_id = channel_id;
	m_marshal_flags = marshal_flags;
	m_message_oid = message_oid;

	if (m_message_oid != guid_t::Null())
		m_ptrOF.Attach(static_cast<Activation::IObjectFactory*>(Activation::GetRegisteredObject(m_message_oid,Activation::InProcess,OMEGA_GUIDOF(Activation::IObjectFactory))));
		
	// Create a new OM
	m_ptrOM = ObjectPtr<Remoting::IObjectManager>(Remoting::OID_StdObjectManager,Activation::InProcess | Activation::DontLaunch);

	// QI for IMarshaller
	m_ptrMarshaller = m_ptrOM;
	if (!m_ptrMarshaller)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller),OMEGA_SOURCE_INFO);

	// Associate it with the channel
	m_ptrOM->Connect(this);
}

void User::Channel::disconnect()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	if (m_ptrOM)
		m_ptrOM->Shutdown();
	
	m_ptrOM.Release();
	m_ptrMarshaller.Release();
}

Remoting::IMessage* User::Channel::CreateMessage()
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

IException* User::Channel::SendAndReceive(TypeInfo::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t timeout)
{
	// Get the object manager
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	ObjectPtr<Remoting::IMarshaller> ptrMarshaller = m_ptrMarshaller;
	if (!m_ptrMarshaller)
		throw Remoting::IChannelClosedException::Create();

	guard.release();

	// We need to wrap the message
	ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrEnvelope = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();
	ptrMarshaller->MarshalInterface(L"payload",ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pSend);

	OOBase::SmartPtr<OOBase::CDRStream> response;
	try
	{
		OOBase::timeval_t deadline;
		if (timeout > 0)
			deadline = OOBase::timeval_t::deadline(timeout);

		Root::MessageHandler::io_result::type res = m_pManager->send_request(m_channel_id,ptrEnvelope->GetCDRStream(),response,timeout ? &deadline : 0,attribs);
		if (res != Root::MessageHandler::io_result::success)
		{
			if (res == Root::MessageHandler::io_result::timedout)
				throw Omega::ITimeoutException::Create();
			else if (res == Root::MessageHandler::io_result::channel_closed)
			{
				disconnect();
				throw Omega::Remoting::IChannelClosedException::Create();
			}
			else
				OMEGA_THROW(L"Internal server exception");
		}
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
			ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrRecv = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();
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

Remoting::MarshalFlags_t User::Channel::GetMarshalFlags()
{
	return m_marshal_flags;
}

uint32_t User::Channel::GetSource()
{
	return m_channel_id;
}

bool_t User::Channel::IsConnected()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);
	
	return (!m_ptrOM ? false : true);
}

guid_t User::Channel::GetReflectUnmarshalFactoryOID()
{
	return GetUnmarshalFactoryOID(guid_t::Null(),0);
}

void User::Channel::ReflectMarshal(Remoting::IMessage* pMessage)
{
	OOBase::SmartPtr<OOBase::CDRStream> response;
	Root::MessageHandler::io_result::type res = m_pManager->send_request(m_channel_id,0,response,0,Root::Message_t::synchronous | Root::Message_t::channel_reflect);
	if (res != Root::MessageHandler::io_result::success)
	{
		if (res == Root::MessageHandler::io_result::timedout)
			throw Omega::ITimeoutException::Create();
		else if (res == Root::MessageHandler::io_result::channel_closed)
			throw Omega::Remoting::IChannelClosedException::Create();
		else
			OMEGA_THROW(L"Internal server exception");
	}

	if (!response)
		OMEGA_THROW(L"No response received");
		
	Omega::uint32_t other_end = 0;
	if (!response->read(other_end))
		OMEGA_THROW(response->last_error());

	// Return in the same format as we marshal
	pMessage->WriteUInt32(L"m_channel_id",other_end);
	pMessage->WriteGuid(L"m_message_oid",m_message_oid);
}

void User::Channel::GetManager(const guid_t& iid, IObject*& pObject)
{
	// Get the object manager
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);
	
	pObject = m_ptrOM->QueryInterface(iid);
}

guid_t User::Channel::GetUnmarshalFactoryOID(const guid_t&, Remoting::MarshalFlags_t)
{
	// This must match OOCore::OID_ChannelMarshalFactory
	return guid_t(L"{7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}");
}

void User::Channel::MarshalInterface(Remoting::IMarshaller*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	pMessage->WriteUInt32(L"m_channel_id",m_channel_id);
	pMessage->WriteGuid(L"m_message_oid",m_message_oid);
}

void User::Channel::ReleaseMarshalData(Remoting::IMarshaller*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	pMessage->ReadUInt32(L"m_channel_id");
	pMessage->ReadGuid(L"m_message_oid");
}

void User::ChannelMarshalFactory::UnmarshalInterface(Remoting::IMarshaller*, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	Omega::uint32_t channel_id = pMessage->ReadUInt32(L"m_channel_id");
	guid_t message_oid = pMessage->ReadGuid(L"m_message_oid");

	// Create a new object manager (and channel)
	pObject = Manager::create_channel(channel_id,message_oid)->QueryInterface(iid);
}

OMEGA_DEFINE_OID(OOCore,OID_CDRMessageMarshalFactory,"{1455FCD0-A49B-4f2a-94A5-222949957123}");
