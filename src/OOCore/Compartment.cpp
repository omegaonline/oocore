///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
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

#include "UserSession.h"
#include "StdObjectManager.h"
#include "Activation.h"
#include "SimpleMarshaller.h"

using namespace Omega;
using namespace OTL;

OOCore::Compartment::Compartment(UserSession* pSession) :
		m_pSession(pSession),
		m_id(0x1000)
{
}

void OOCore::Compartment::set_id(uint16_t id)
{
	m_id = id;
}

OOCore::Compartment::ComptState::ComptState(Compartment* cmpt) : m_cmpt(cmpt)
{
	m_prev_id = m_cmpt->m_pSession->update_state(m_cmpt->m_id);
}

OOCore::Compartment::ComptState::~ComptState()
{
	m_cmpt->m_pSession->update_state(m_prev_id);
}

void OOCore::Compartment::shutdown()
{
	// Switch state...
	ComptState compt_state(this);

	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Done with cached objects
	m_ptrROT.Release();

	m_pSession->remove_compartment(m_id);
	uint32_t our_channel_id = m_id | m_pSession->get_channel_id();

	// Shutdown all channels and compartments
	ChannelInfo info;
	while (m_mapChannels.pop(NULL,&info))
	{
/*		guard.release();

		if (info.m_bOpen)
			info.m_ptrChannel->shutdown(our_channel_id);
		else
			info.m_ptrChannel->on_disconnect();

		guard.acquire();*/
	}

	ObjectPtr<ObjectImpl<ComptChannel> > ptrCompt;
	while (m_mapCompartments.pop(NULL,&ptrCompt))
	{
		guard.release();

		ptrCompt->shutdown();

		guard.acquire();
	}		
}

void OOCore::Compartment::process_compartment_close(uint16_t src_compt_id)
{
	// Switch state...
	ComptState compt_state(this);

	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	ObjectPtr<ObjectImpl<ComptChannel> > ptrChannel;
	m_mapCompartments.remove(src_compt_id,&ptrChannel);
		
	guard.release();

	if (ptrChannel)
		ptrChannel->on_disconnect();
}

bool OOCore::Compartment::process_channel_close(uint32_t closed_channel_id)
{
	// Close the corresponding Object Manager
	bool bRet = false;
	
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	for (OOBase::HashTable<uint32_t,ChannelInfo>::iterator i=m_mapChannels.begin();i!=m_mapChannels.end();++i)
	{
		if (i->key == closed_channel_id)
		{
			// Close if its an exact match
			i->value.m_bOpen = false;
			bRet = true;
		}
		else if (!(closed_channel_id & 0xFFF) && (i->key & 0xFFFFF000) == closed_channel_id)
		{
			// Close all compartments on the channel if 0 cmpt closes
			i->value.m_bOpen = false;
			bRet = true;
		}
	}
	
	return bRet;
}

bool OOCore::Compartment::is_channel_open(uint32_t channel_id)
{
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	OOBase::HashTable<uint32_t,ChannelInfo>::iterator i = m_mapChannels.find(channel_id);
	if (i == m_mapChannels.end())
		return false;

	if (i->value.m_bOpen)
		return true;

	read_guard.release();

	// Now remove the closed channel
	OOBase::Guard<OOBase::RWMutex> write_guard(m_lock);
	
	i = m_mapChannels.find(channel_id);
	if (i != m_mapChannels.end() && i->value.m_bOpen)
		m_mapChannels.remove_at(i);

	return false;
}

Remoting::IObjectManager* OOCore::Compartment::get_channel_om(uint32_t src_channel_id)
{
	// Lookup existing..
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	OOBase::HashTable<uint32_t,ChannelInfo>::iterator i = m_mapChannels.find(src_channel_id);
	if (i == m_mapChannels.end())
		OMEGA_THROW(ENOENT);

	ObjectPtr<Remoting::IChannel> ptrChannel = i->value.m_ptrChannel;

	read_guard.release();

	return NULL;//ptrChannel->GetObjectManager();
}

ObjectImpl<OOCore::Channel>* OOCore::Compartment::create_channel(uint32_t src_channel_id, const guid_t& message_oid, Remoting::MarshalFlags_t flags)
{
	// Lookup existing..
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	ObjectPtr<Remoting::IChannel> ptrChannel;
	OOBase::HashTable<uint32_t,ChannelInfo>::iterator i = m_mapChannels.find(src_channel_id);
	if (i != m_mapChannels.end())
		ptrChannel = i->value.m_ptrChannel;
	else
	{
		read_guard.release();

		// Create a new OM
		ObjectPtr<ObjectImpl<StdObjectManager> > ptrOM = ObjectImpl<StdObjectManager>::CreateObject();

		// Create a new channel
		ObjectPtr<ObjectImpl<Channel> > ptrChannel2 = ObjectImpl<Channel>::CreateObject();
		ptrChannel2->init(m_pSession,flags,src_channel_id,ptrOM,message_oid);

		// And add to the map
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		ChannelInfo info;
		info.m_bOpen = true;
		info.m_ptrChannel = ptrChannel2;
		ptrChannel = info.m_ptrChannel;

		int err = 0;
		i = m_mapChannels.insert(src_channel_id,info,err);
		if (err == EEXIST)
			ptrChannel = i->value.m_ptrChannel;
		else if (err)
			OMEGA_THROW(err);
	}
		
	return NULL;//m_ptrChannel.AddRef();
}

Remoting::IObjectManager* OOCore::Compartment::unmarshal_om(Remoting::IMessage* pMessage)
{
	// Create a simple marshal context
	ObjectPtr<ObjectImpl<SimpleMarshalContext> > ptrMarshalContext = ObjectImpl<SimpleMarshalContext>::CreateObject();
	ptrMarshalContext->init(Remoting::Same);

	// Unmarshal the embedded channel
	ObjectPtr<Remoting::IChannel> ptrChannel;
	ptrChannel.Unmarshal(ptrMarshalContext,string_t::constant("channel"),pMessage);

	// Create a new OM
	ObjectPtr<ObjectImpl<StdObjectManager> > ptrOM = ObjectImpl<StdObjectManager>::CreateObject();

	ChannelInfo info;
	info.m_bOpen = true;
	info.m_ptrChannel = ptrChannel;

	// And add to the map
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	int err = m_mapChannels.insert(ptrChannel->GetSource(),info);
	if (err)
		OMEGA_THROW(err);
		
	return NULL;//info.m_ptrChannel.AddRef();
}

void OOCore::Compartment::process_request(const Message& msg)
{
	// Find and/or create the object manager associated with src_channel_id
	ObjectPtr<Remoting::IObjectManager> ptrOM = get_channel_om(msg.m_src_channel_id);

	// QI for IMarshalContext
	ObjectPtr<Remoting::IMarshalContext> ptrMarshalContext = ptrOM.QueryInterface<Remoting::IMarshalContext>();
	if (!ptrMarshalContext)
		throw OOCore_INotFoundException_MissingIID(OMEGA_GUIDOF(Remoting::IMarshalContext));

	// Wrap up the request
	ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrEnvelope = ObjectImpl<OOCore::CDRMessage>::CreateObject();
	ptrEnvelope->init(msg.m_payload);

	// Unpack the payload
	ObjectPtr<Remoting::IMessage> ptrRequest;
	ptrRequest.Unmarshal(ptrMarshalContext,string_t::constant("payload"),ptrEnvelope);

	// Make the call
	ObjectPtr<Remoting::IMessage> ptrResult = ptrOM->Invoke(ptrRequest);

	if (!(msg.m_attribs & TypeInfo::Asynchronous))
	{
		// Wrap the response...
		ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrResponse = ObjectImpl<OOCore::CDRMessage>::CreateObject();
		ptrMarshalContext->MarshalInterface(string_t::constant("payload"),ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);

		// Send it back...
		try
		{
			m_pSession->send_response(msg,ptrResponse->GetCDRStream());
		}
		catch (...)
		{
			ptrMarshalContext->ReleaseMarshalData(string_t::constant("payload"),ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);
			throw;
		}
	}
}

ObjectImpl<OOCore::ComptChannel>* OOCore::Compartment::create_compartment_channel(uint16_t compartment_id, const guid_t& message_oid)
{
	// Lookup existing..
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	ObjectPtr<ObjectImpl<ComptChannel> > ptrChannel;
	OOBase::HashTable<uint16_t,OTL::ObjectPtr<OTL::ObjectImpl<ComptChannel> > >::iterator i = m_mapCompartments.find(compartment_id);
	if (i != m_mapCompartments.end())
		ptrChannel = i->value;
	
	read_guard.release();

	if (!ptrChannel)
	{
		// Get the compartment
		OOBase::SmartPtr<Compartment> ptrCompt = m_pSession->get_compartment(compartment_id);
		if (!ptrCompt)
			throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("Failed to find compartment in session"));

		// Create a new channel
		ptrChannel = ObjectImpl<ComptChannel>::CreateObject();
		
		ObjectPtr<Remoting::IObjectManager> ptrOM = ObjectImpl<StdObjectManager>::CreateObject();
		ptrChannel->init(m_id,ptrCompt,compartment_id | m_pSession->get_channel_id(),ptrOM,message_oid);

		// And add to the map
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		int err = 0;
		i = m_mapCompartments.insert(compartment_id,ptrChannel,err);
		if (err == EEXIST)
			ptrChannel = i->value;
		else if (err != 0)
			OMEGA_THROW(err);
	}

	return ptrChannel.Detach();
}

IException* OOCore::Compartment::compartment_message(uint16_t src_cmpt_id, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv)
{
	// Switch state...
	ComptState compt_state(this);
	
	IException* pRet = 0;
	try
	{
		// Find and/or create the object manager associated with src_channel_id
		ObjectPtr<ObjectImpl<ComptChannel> > ptrChannel = create_compartment_channel(src_cmpt_id,guid_t::Null());
		ObjectPtr<Remoting::IObjectManager> ptrOM = ptrChannel->GetObjectManager();

		// Make the call
		pRecv = ptrOM->Invoke(pSend);
	}
	catch (IException* pE)
	{
		pRet = pE;
	}

	return pRet;
}

void OOCore::ComptChannel::init(uint16_t src_compt_id, const OOBase::SmartPtr<Compartment>& ptrCompt, uint32_t channel_id, Remoting::IObjectManager* pOM, const guid_t& message_oid)
{
	ChannelBase::init(channel_id,Remoting::Compartment,pOM,message_oid);

	m_src_compt_id = src_compt_id;
	m_ptrCompt = ptrCompt;	
}

void OOCore::ComptChannel::shutdown()
{
	m_ptrCompt->process_compartment_close(m_src_compt_id);

	on_disconnect();
}

bool_t OOCore::ComptChannel::IsConnected()
{
	return true;
}

IException* OOCore::ComptChannel::SendAndReceive(TypeInfo::MethodAttributes_t /*attribs*/, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv)
{
	return m_ptrCompt->compartment_message(m_src_compt_id,pSend,pRecv);
}

namespace OOCore
{
	class CompartmentImpl :
			public ObjectBase,
			public Omega::Compartment::ICompartment
	{
	public:
		void init(ObjectPtr<ObjectImpl<OOCore::ComptChannel> > ptrChannel);

		BEGIN_INTERFACE_MAP(CompartmentImpl)
			INTERFACE_ENTRY(Omega::Compartment::ICompartment)
		END_INTERFACE_MAP()

	private:
		OOBase::SpinLock                             m_lock;
		ObjectPtr<ObjectImpl<OOCore::ComptChannel> > m_ptrChannel;

		void Final_Release();

	// ICompartment members
	public:
		void CreateObject(const any_t& oid, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject);
	};
}

void OOCore::CompartmentImpl::Final_Release()
{
	m_ptrChannel->shutdown();

	delete this;
}

void OOCore::CompartmentImpl::init(ObjectPtr<ObjectImpl<OOCore::ComptChannel> > ptrChannel)
{
	m_ptrChannel = ptrChannel;
}

void OOCore::CompartmentImpl::CreateObject(const any_t& oid, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject)
{
	ObjectPtr<Remoting::IObjectManager> ptrOM = m_ptrChannel->GetObjectManager();

	// Get the remote instance IObjectFactory
	IObject* pObjF = NULL;
	ptrOM->GetRemoteInstance(oid,flags,OMEGA_GUIDOF(Activation::IObjectFactory),pObjF);
	ObjectPtr<Activation::IObjectFactory> ptrOF = static_cast<Activation::IObjectFactory*>(pObjF);
	
	// Call CreateObject
	ptrOF->CreateObject(iid,pObject);
}

// {3BE419D7-52D9-4873-95E7-836D33523C51}
OMEGA_DEFINE_OID(Compartment,OID_Compartment,"{3BE419D7-52D9-4873-95E7-836D33523C51}");

void OOCore::CompartmentFactory::CreateObject(const guid_t& iid, IObject*& pObject)
{
	// Create a new compartment and get the channel to it...
	ObjectPtr<ObjectImpl<OOCore::ComptChannel> > ptrChannel = OOCore::UserSession::create_compartment(guid_t::Null());

	// Create a CompartmentImpl
	ObjectPtr<ObjectImpl<OOCore::CompartmentImpl> > ptrCompt = ObjectImpl<OOCore::CompartmentImpl>::CreateObject();
	ptrCompt->init(ptrChannel);

	pObject = ptrCompt->QueryInterface(iid);
}
