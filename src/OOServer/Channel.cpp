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

#include "./OOServer_User.h"
#include "./Channel.h"
#include "./UserManager.h"

using namespace Omega;
using namespace OTL;

OMEGA_DEFINE_OID(User,OID_ChannelMarshalFactory,"{1A7672C5-8478-4e5a-9D8B-D5D019E25D15}");

User::Channel::Channel() :
	m_channel_id(0),
	m_marshal_flags(0)
{
}

void User::Channel::init(ACE_CDR::ULong channel_id, Remoting::MarshalFlags_t marshal_flags, const guid_t& message_oid, Omega::Remoting::IObjectManager* pOM)
{
	m_channel_id = channel_id;
	m_marshal_flags = marshal_flags;
	m_message_oid = message_oid;
	m_pOM = pOM;
	System::PinObjectPointer(m_pOM);
}

void User::Channel::disconnect()
{
	m_channel_id = 0;
	System::UnpinObjectPointer(m_pOM);
	m_pOM = 0;
}

Remoting::IMessage* User::Channel::CreateMessage()
{
	if (!m_channel_id)
		OMEGA_THROW(ECONNRESET);

	if (m_message_oid == guid_t::Null())
	{
		// Create a fresh OutputCDR
		ObjectPtr<ObjectImpl<OOCore::OutputCDR> > ptrOutput = ObjectImpl<OOCore::OutputCDR>::CreateInstancePtr();
		return ptrOutput.QueryInterface<Remoting::IMessage>();
	}
	else
	{
		// Create a fresh one
		ObjectPtr<Remoting::IMessage> ptrMessage(m_message_oid,Activation::InProcess);
		return ptrMessage.AddRef();
	}
}

void User::Channel::Reflect(Remoting::IMessage* pMessage)
{
	if (!m_channel_id)
		OMEGA_THROW(ECONNRESET);

	ACE_InputCDR* response = 0;

	if (!User::Manager::USER_MANAGER::instance()->send_request(m_channel_id,0,response,0,Root::Message_t::synchronous | Root::Message_t::channel_reflect))
		OMEGA_THROW(ACE_OS::last_error());

	ACE_CDR::ULong other_end = 0;
	response->read_ulong(other_end);

	// Return in the same format as we marshal
	pMessage->WriteUInt32s(L"m_channel_id",1,&other_end);
	pMessage->WriteGuids(L"m_message_oid",1,&m_message_oid);
}

IException* User::Channel::SendAndReceive(Remoting::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t timeout)
{
	if (!m_channel_id)
		OMEGA_THROW(ECONNRESET);

	// We need to wrap the message
	ObjectPtr<ObjectImpl<OOCore::OutputCDR> > ptrEnvelope = ObjectImpl<OOCore::OutputCDR>::CreateInstancePtr();
	m_pOM->MarshalInterface(L"payload",ptrEnvelope,OMEGA_UUIDOF(Remoting::IMessage),pSend);

	// QI pSend for our private interface
	ObjectPtr<OOCore::IMessageBlockHolder> ptrOutput;
	ptrOutput.Attach(static_cast<OOCore::IMessageBlockHolder*>(ptrEnvelope->QueryInterface(OMEGA_UUIDOF(OOCore::IMessageBlockHolder))));
	if (!ptrOutput)
	{
		m_pOM->ReleaseMarshalData(L"payload",ptrEnvelope,OMEGA_UUIDOF(Remoting::IMessage),pSend);
		OMEGA_THROW(EINVAL);
	}

	// Get the message block
	const ACE_Message_Block* request = static_cast<const ACE_Message_Block*>(ptrOutput->GetMessageBlock());

	ACE_Time_Value deadline;
	if (timeout > 0)
		deadline = ACE_OS::gettimeofday() + ACE_Time_Value(timeout/1000,(timeout % 1000) * 1000);

	ACE_InputCDR* response = 0;
	if (!User::Manager::USER_MANAGER::instance()->send_request(m_channel_id,request,response,timeout ? &deadline : 0,attribs))
	{
		if (m_pOM)
			m_pOM->ReleaseMarshalData(L"payload",ptrEnvelope,OMEGA_UUIDOF(Remoting::IMessage),pSend);
	
		OMEGA_THROW(ACE_OS::last_error());
	}

	try
	{
		if (response)
		{
			// Wrap the response
			ObjectPtr<ObjectImpl<OOCore::InputCDR> > ptrRecv = ObjectImpl<OOCore::InputCDR>::CreateInstancePtr();
			ptrRecv->init(*response);
			delete response;

			// Unwrap the payload...
			IObject* pRet = 0;
			m_pOM->UnmarshalInterface(L"payload",ptrRecv,OMEGA_UUIDOF(Remoting::IMessage),pRet);
			pRecv = static_cast<Remoting::IMessage*>(pRet);
		}
	}
	catch (IException* pE)
	{
		delete response;
		return pE;
	}
	catch (...)
	{
		delete response;
		throw;
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

guid_t User::Channel::GetUnmarshalFactoryOID(const guid_t&, Remoting::MarshalFlags_t)
{
	// This must match OOCore::OID_ChannelMarshalFactory
	static const guid_t oid = guid_t::FromString(L"{7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}");
	return oid;
}

void User::Channel::MarshalInterface(Remoting::IObjectManager*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	pMessage->WriteUInt32s(L"m_channel_id",1,&m_channel_id);

	printf("Marshalling %#x\n",m_channel_id);

	pMessage->WriteGuids(L"m_message_oid",1,&m_message_oid);
}

void User::Channel::ReleaseMarshalData(Remoting::IObjectManager*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	uint32_t c;
	if (pMessage->ReadUInt32s(L"m_channel_id",1,&c) != 1)
		OMEGA_THROW(EIO);

	guid_t g;
	if (pMessage->ReadGuids(L"m_message_oid",1,&g) != 1)
		OMEGA_THROW(EIO);
}

void User::ChannelMarshalFactory::UnmarshalInterface(Remoting::IObjectManager*, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	ACE_CDR::ULong channel_id;
	if (pMessage->ReadUInt32s(L"m_channel_id",1,&channel_id) != 1)
		OMEGA_THROW(EIO);

	guid_t message_oid;
	if (pMessage->ReadGuids(L"m_message_oid",1,&message_oid) != 1)
		OMEGA_THROW(EIO);

	// Create a new object manager (and channel)
	pObject = Manager::USER_MANAGER::instance()->create_object_manager(channel_id,message_oid)->QueryInterface(iid);
}
