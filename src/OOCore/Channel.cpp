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

OOCore::Channel::Channel() :
	m_pSession(0),
	m_channel_id(0),
	m_marshal_flags(0)
{
}

void OOCore::Channel::init(UserSession* pSession, uint16_t apt_id, uint32_t channel_id, Remoting::MarshalFlags_t marshal_flags, const guid_t& message_oid, Remoting::IObjectManager* pOM)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	m_pSession = pSession;
	m_apt_id = apt_id;
	m_channel_id = channel_id;
	m_marshal_flags = marshal_flags;
	m_message_oid = message_oid;

	if (m_message_oid != guid_t::Null())
		m_ptrOF.Attach(static_cast<Activation::IObjectFactory*>(Activation::GetRegisteredObject(m_message_oid,Activation::InProcess,OMEGA_GUIDOF(Activation::IObjectFactory))));
	
	// Connect the OM to us
	m_ptrOM = pOM;
	m_ptrOM->Connect(this);
}

void OOCore::Channel::disconnect()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	if (m_ptrOM)
	{
		m_ptrOM->Shutdown();
		m_ptrOM.Release();
	}
}

Remoting::IMessage* OOCore::Channel::CreateMessage()
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

IException* OOCore::Channel::SendAndReceive(TypeInfo::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv,  uint32_t timeout)
{
	// Get the object manager
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);
	if (!m_ptrOM)
		throw Remoting::IChannelClosedException::Create();
	
	ObjectPtr<Remoting::IObjectManager> ptrOM = m_ptrOM;
	guard.release();

	// We need to wrap the message
	ObjectPtr<ObjectImpl<CDRMessage> > ptrEnvelope = ObjectImpl<CDRMessage>::CreateInstancePtr();
	ptrOM->MarshalInterface(L"payload",ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pSend);

	OOBase::SmartPtr<OOBase::CDRStream> response = 0;
	try
	{
		// QI pSend for our private interface
		ObjectPtr<ICDRStreamHolder> ptrOutput;
		ptrOutput.Attach(static_cast<ICDRStreamHolder*>(ptrEnvelope->QueryInterface(OMEGA_GUIDOF(ICDRStreamHolder))));
		assert(ptrOutput);
		
		// Get the buffer
		const OOBase::CDRStream* request = static_cast<const OOBase::CDRStream*>(ptrOutput->GetCDRStream());

		response = m_pSession->send_request(m_apt_id,m_channel_id,request,timeout,attribs);
	}
	catch (Remoting::IChannelClosedException* pE)
	{
		ptrOM->ReleaseMarshalData(L"payload",ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pSend);
		disconnect();
		throw pE;
	}
	catch (...)
	{
		ptrOM->ReleaseMarshalData(L"payload",ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pSend);
		throw;
	}

	if (response)
	{
		// Wrap the response
		ObjectPtr<ObjectImpl<CDRMessage> > ptrRecv = ObjectImpl<CDRMessage>::CreateInstancePtr();
		ptrRecv->init(*response);
		
		// Unwrap the payload...
		IObject* pRet = 0;
		ptrOM->UnmarshalInterface(L"payload",ptrRecv,OMEGA_GUIDOF(Remoting::IMessage),pRet);
		pRecv = static_cast<Remoting::IMessage*>(pRet);
	}
	
	return 0;
}

Remoting::MarshalFlags_t OOCore::Channel::GetMarshalFlags()
{
	return m_marshal_flags;
}

uint32_t OOCore::Channel::GetSource()
{
	return m_channel_id;
}

bool_t OOCore::Channel::IsConnected()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);
	
	return (!m_ptrOM ? false : true);
}

guid_t OOCore::Channel::GetReflectUnmarshalFactoryOID()
{
	return OID_ChannelMarshalFactory;
}

void OOCore::Channel::ReflectMarshal(Remoting::IMessage* pMessage)
{
	OOBase::SmartPtr<OOBase::CDRStream> response = 0;
	
	try
	{
		response = m_pSession->send_request(m_apt_id,m_channel_id,0,0,Message::synchronous | Message::channel_reflect);
	}
	catch (...)
	{
		// Disconnect ourselves on failure
		disconnect();
		throw;
	}
	
	uint32_t other_end = 0;
	if (response && !response->read(other_end))
		OMEGA_THROW(EIO);
	
	// Return in the same format as we marshal
	pMessage->WriteUInt32s(L"m_channel_id",1,&other_end);
	pMessage->WriteGuids(L"m_message_oid",1,&m_message_oid);
}

Remoting::IObjectManager* OOCore::Channel::GetObjectManager()
{
	// Get the object manager
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);
	
	return m_ptrOM.AddRef();
}

guid_t OOCore::Channel::GetUnmarshalFactoryOID(const guid_t&, Remoting::MarshalFlags_t)
{
	return OID_ChannelMarshalFactory;
}

void OOCore::Channel::MarshalInterface(Remoting::IObjectManager*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	pMessage->WriteUInt32s(L"m_channel_id",1,&m_channel_id);
	pMessage->WriteGuids(L"m_message_oid",1,&m_message_oid);
}

void OOCore::Channel::ReleaseMarshalData(Remoting::IObjectManager*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	uint32_t c;
	if (pMessage->ReadUInt32s(L"m_channel_id",1,&c) != 1)
		OMEGA_THROW(EIO);

	guid_t g;
	if (pMessage->ReadGuids(L"m_message_oid",1,&g) != 1)
		OMEGA_THROW(EIO);
}

OMEGA_DEFINE_OID(OOCore,OID_ChannelMarshalFactory,"{7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}");

void OOCore::ChannelMarshalFactory::UnmarshalInterface(Remoting::IObjectManager* pObjectManager, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t flags, IObject*& pObject)
{
	if (OOCore::HostedByOOServer())
	{
		// This must match OOServer::User::OID_ChannelMarshalFactory
		static const guid_t oid = guid_t::FromString(L"{1A7672C5-8478-4e5a-9D8B-D5D019E25D15}");
		ObjectPtr<Remoting::IMarshalFactory> ptrMarshalFactory(oid,Activation::InProcess | Activation::DontLaunch);

		// If we have a pointer by now then we are actually running in the OOServer.exe,
		// and can therefore do our specialized unmarshalling...
		return ptrMarshalFactory->UnmarshalInterface(pObjectManager,pMessage,iid,flags,pObject);
	}
	else
	{
		// If we get here, then we are loaded into a different exe from OOServer,
		// therefore we do simple unmarshalling
	
		uint32_t channel_id;
		if (pMessage->ReadUInt32s(L"m_channel_id",1,&channel_id) != 1)
			OMEGA_THROW(EIO);

		guid_t message_oid;
		if (pMessage->ReadGuids(L"m_message_oid",1,&message_oid) != 1)
			OMEGA_THROW(EIO);

		// Create a new channel
		pObject = UserSession::create_channel(channel_id,message_oid)->QueryInterface(iid);
	}
}

OMEGA_DEFINE_OID(OOCore,OID_CDRMessageMarshalFactory,"{1455FCD0-A49B-4f2a-94A5-222949957123}");

void OOCore::CDRMessageMarshalFactory::UnmarshalInterface(Remoting::IObjectManager*, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	uint64_t sz;
	pMessage->ReadUInt64s(L"length",1,&sz);

	if (sz > (size_t)-1)
		OMEGA_THROW(E2BIG);
	size_t len = static_cast<size_t>(sz);

	OOBase::CDRStream input(len);
	pMessage->ReadBytes(L"data",len,(byte_t*)input.buffer()->wr_ptr());
	input.buffer()->wr_ptr(len);
	
	ObjectPtr<ObjectImpl<CDRMessage> > ptrInput = ObjectImpl<CDRMessage>::CreateInstancePtr();
	ptrInput->init(input);

	pObject = ptrInput->QueryInterface(iid);
}
