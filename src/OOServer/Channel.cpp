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

	// Associate it with the channel
	m_ptrOM->Connect(this);
}

void User::Channel::disconnect()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	if (m_ptrOM)
	{
		m_ptrOM->Shutdown();
		m_ptrOM.Release();
	}
}

Remoting::IMessage* User::Channel::CreateMessage()
{
	if (m_message_oid == guid_t::Null())
	{
		// Create a fresh CDRMessage
		ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrOutput = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();
		return ptrOutput.QueryInterface<Remoting::IMessage>();
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
	if (!m_ptrOM)
		throw Omega::Remoting::IChannelClosedException::Create();
	
	ObjectPtr<Remoting::IObjectManager> ptrOM = m_ptrOM;
	guard.release();

	// We need to wrap the message
	ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrEnvelope = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();
	ptrOM->MarshalInterface(L"payload",ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pSend);

	OOBase::SmartPtr<OOBase::CDRStream> response;
	try
	{
		// QI pSend for our private interface
		ObjectPtr<OOCore::ICDRStreamHolder> ptrOutput;
		ptrOutput.Attach(static_cast<OOCore::ICDRStreamHolder*>(ptrEnvelope->QueryInterface(OMEGA_GUIDOF(OOCore::ICDRStreamHolder))));
		assert(ptrOutput);
		
		// Get the message block
		const OOBase::CDRStream* request = static_cast<const OOBase::CDRStream*>(ptrOutput->GetCDRStream());

		OOBase::timeval_t deadline;
		if (timeout > 0)
			deadline = OOBase::timeval_t::deadline(timeout);

		Root::MessageHandler::io_result::type res = m_pManager->send_request(m_channel_id,request,response,timeout ? &deadline : 0,attribs);
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
		ptrOM->ReleaseMarshalData(L"payload",ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pSend);
		throw;
	}

	try
	{
		if (response)
		{
			// Wrap the response
			ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrRecv = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();
			ptrRecv->init(*response);
					
			// Unwrap the payload...
			IObject* pRet = 0;
			ptrOM->UnmarshalInterface(L"payload",ptrRecv,OMEGA_GUIDOF(Remoting::IMessage),pRet);
			pRecv = static_cast<Remoting::IMessage*>(pRet);
		}
	}
	catch (IException* pE)
	{
		return pE;
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
	pMessage->WriteUInt32s(L"m_channel_id",1,&other_end);
	pMessage->WriteGuids(L"m_message_oid",1,&m_message_oid);
}

Remoting::IObjectManager* User::Channel::GetObjectManager()
{
	// Get the object manager
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);
	
	return m_ptrOM.AddRef();
}

guid_t User::Channel::GetUnmarshalFactoryOID(const guid_t&, Remoting::MarshalFlags_t)
{
	// This must match OOCore::OID_ChannelMarshalFactory
	static const guid_t oid = guid_t::FromString(L"{7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}");
	return oid;
}

void User::Channel::MarshalInterface(Remoting::IObjectManager*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	pMessage->WriteUInt32s(L"m_channel_id",1,&m_channel_id);
	pMessage->WriteGuids(L"m_message_oid",1,&m_message_oid);
}

void User::Channel::ReleaseMarshalData(Remoting::IObjectManager*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	uint32_t c;
	if (pMessage->ReadUInt32s(L"m_channel_id",1,&c) != 1)
		OMEGA_THROW(L"Unexpected end of message");

	guid_t g;
	if (pMessage->ReadGuids(L"m_message_oid",1,&g) != 1)
		OMEGA_THROW(L"Unexpected end of message");
}

void User::ChannelMarshalFactory::UnmarshalInterface(Remoting::IObjectManager*, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	Omega::uint32_t channel_id;
	if (pMessage->ReadUInt32s(L"m_channel_id",1,&channel_id) != 1)
		OMEGA_THROW(L"Unexpected end of message");

	guid_t message_oid;
	if (pMessage->ReadGuids(L"m_message_oid",1,&message_oid) != 1)
		OMEGA_THROW(L"Unexpected end of message");

	// Create a new object manager (and channel)
	pObject = Manager::create_channel(channel_id,message_oid)->QueryInterface(iid);
}

OMEGA_DEFINE_OID(OOCore,OID_CDRMessageMarshalFactory,"{1455FCD0-A49B-4f2a-94A5-222949957123}");
