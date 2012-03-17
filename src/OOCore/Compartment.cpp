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

using namespace Omega;
using namespace OTL;

OOCore::Compartment::Compartment(UserSession* pSession) :
		m_pSession(pSession),
		m_id(0x1000)
{
}

void OOCore::Compartment::set_id(Omega::uint16_t id)
{
	m_id = id;
}

OOCore::Compartment::ComptState::ComptState(Compartment* cmpt, OOBase::Timeout* pTimeout) : m_cmpt(cmpt)
{
	m_prev_id = m_cmpt->m_pSession->update_state(m_cmpt->m_id,pTimeout);
}

OOCore::Compartment::ComptState::~ComptState()
{
	m_cmpt->m_pSession->update_state(m_prev_id,NULL);
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
		guard.release();

		if (info.m_bOpen)
			info.m_ptrChannel->shutdown(our_channel_id);
		else
			info.m_ptrChannel->disconnect();

		guard.acquire();
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

	ObjectPtr<ObjectImpl<ComptChannel> > ptrCompt;
	m_mapCompartments.remove(src_compt_id,&ptrCompt);
		
	guard.release();

	if (ptrCompt)
		ptrCompt->disconnect();
}

bool OOCore::Compartment::process_channel_close(uint32_t closed_channel_id)
{
	// Close the corresponding Object Manager
	bool bRet = false;
	
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	for (size_t i=m_mapChannels.begin();i!=m_mapChannels.npos;i=m_mapChannels.next(i))
	{
		uint32_t k = *m_mapChannels.key_at(i);
		ChannelInfo* pInfo = m_mapChannels.at(i);

		if (k == closed_channel_id)
		{
			// Close if its an exact match
			pInfo->m_bOpen = false;
			bRet = true;
		}
		else if (!(closed_channel_id & 0xFFF) && (k & 0xFFFFF000) == closed_channel_id)
		{
			// Close all compartments on the channel if 0 cmpt closes
			pInfo->m_bOpen = false;
			bRet = true;
		}
	}
	
	return bRet;
}

bool OOCore::Compartment::is_channel_open(uint32_t channel_id)
{
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	ChannelInfo info;
	if (!m_mapChannels.find(channel_id,info))
		return false;

	if (info.m_bOpen)
		return true;

	read_guard.release();

	// Now remove the closed channel
	OOBase::Guard<OOBase::RWMutex> write_guard(m_lock);

	if (m_mapChannels.find(channel_id,info) && !info.m_bOpen)
		m_mapChannels.remove(channel_id);
	
	return false;
}

Remoting::IObjectManager* OOCore::Compartment::get_channel_om(uint32_t src_channel_id)
{
	ObjectPtr<ObjectImpl<OOCore::Channel> > ptrChannel = create_channel(src_channel_id,guid_t::Null());

	return ptrChannel->GetObjectManager();
}

ObjectImpl<OOCore::Channel>* OOCore::Compartment::create_channel(uint32_t src_channel_id, const guid_t& message_oid)
{
	// Lookup existing..
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	ChannelInfo info;
	if (!m_mapChannels.find(src_channel_id,info))
	{
		read_guard.release();

		// Create a new OM
		ObjectPtr<ObjectImpl<StdObjectManager> > ptrOM = ObjectImpl<StdObjectManager>::CreateInstance();

		// Create a new channel
		ObjectPtr<ObjectImpl<Channel> > ptrChannel = ObjectImpl<Channel>::CreateInstance();
		ptrChannel->init(m_pSession,src_channel_id,ptrOM,message_oid);

		// And add to the map
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		info.m_bOpen = true;
		info.m_ptrChannel = ptrChannel;

		int err = m_mapChannels.insert(src_channel_id,info);
		if (err == EEXIST)
			m_mapChannels.find(src_channel_id,info);
		else if (err != 0)
			OMEGA_THROW(err);
	}
		
	return info.m_ptrChannel.AddRef();
}

void OOCore::Compartment::process_request(const Message& msg, const OOBase::Timeout& timeout)
{
	assert(msg.m_dest_cmpt_id == m_id);

	// Find and/or create the object manager associated with src_channel_id
	ObjectPtr<Remoting::IObjectManager> ptrOM = get_channel_om(msg.m_src_channel_id);

	// QI for IMarshaller
	ObjectPtr<Remoting::IMarshaller> ptrMarshaller = ptrOM.QueryInterface<Remoting::IMarshaller>();
	if (!ptrMarshaller)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller));

	// Wrap up the request
	ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrEnvelope = ObjectImpl<OOCore::CDRMessage>::CreateInstance();
	ptrEnvelope->init(msg.m_payload);

	// Unpack the payload
	ObjectPtr<Remoting::IMessage> ptrRequest;
	ptrRequest.Unmarshal(ptrMarshaller,string_t::constant("payload"),ptrEnvelope);

	// Check timeout - at the last possible moment...
	if (timeout.has_expired())
		throw ITimeoutException::Create();

	// Make the call
	ObjectPtr<Remoting::IMessage> ptrResult = ptrOM->Invoke(ptrRequest,timeout.millisecs());

	if (!(msg.m_attribs & TypeInfo::Asynchronous))
	{
		// Wrap the response...
		ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrResponse = ObjectImpl<OOCore::CDRMessage>::CreateInstance();
		ptrMarshaller->MarshalInterface(string_t::constant("payload"),ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);

		// Send it back...
		try
		{
			m_pSession->send_response(msg.m_dest_cmpt_id,msg.m_src_channel_id,msg.m_src_thread_id,ptrResponse->GetCDRStream(),timeout);
		}
		catch (...)
		{
			ptrMarshaller->ReleaseMarshalData(string_t::constant("payload"),ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);
			throw;
		}
	}
}

ObjectImpl<OOCore::ComptChannel>* OOCore::Compartment::create_compartment_channel(uint16_t compartment_id, const guid_t& message_oid)
{
	// Lookup existing..
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	ObjectPtr<ObjectImpl<ComptChannel> > ptrChannel;
	m_mapCompartments.find(compartment_id,ptrChannel);
	
	read_guard.release();

	if (!ptrChannel)
	{
		// Get the compartment
		OOBase::SmartPtr<Compartment> ptrCompt = m_pSession->get_compartment(compartment_id);
		if (!ptrCompt)
			throw Remoting::IChannelClosedException::Create(OMEGA_CREATE_INTERNAL("Failed to find compartment in session"));

		// Create a new channel
		ptrChannel = ObjectImpl<ComptChannel>::CreateInstance();
		
		ObjectPtr<Remoting::IObjectManager> ptrOM = ObjectImpl<StdObjectManager>::CreateInstance();
		ptrChannel->init(m_id,ptrCompt,compartment_id | m_pSession->get_channel_id(),ptrOM,message_oid);

		// And add to the map
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		int err = m_mapCompartments.insert(compartment_id,ptrChannel);
		if (err == EEXIST)
			m_mapCompartments.find(compartment_id,ptrChannel);
		else if (err != 0)
			OMEGA_THROW(err);
	}

	return ptrChannel.Detach();
}

IException* OOCore::Compartment::compartment_message(Omega::uint16_t src_cmpt_id, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t millisecs)
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
		pRecv = ptrOM->Invoke(pSend,millisecs);
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

	disconnect();
}

Omega::bool_t OOCore::ComptChannel::IsConnected()
{
	return true;
}

IException* OOCore::ComptChannel::SendAndReceive(TypeInfo::MethodAttributes_t /*attribs*/, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t millisecs)
{
	return m_ptrCompt->compartment_message(m_src_compt_id,pSend,pRecv,millisecs);
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
		void CreateInstance(const any_t& oid, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid, IObject*& pObject);
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

void OOCore::CompartmentImpl::CreateInstance(const any_t& oid, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid, IObject*& pObject)
{
	if (pOuter && iid != OMEGA_GUIDOF(Omega::IObject))
		throw Omega::IInternalException::Create("Aggregation must use iid of OMEGA_GUIDOF(Omega::IObject)","Omega::ICompartment::CreateInstance");
	
	ObjectPtr<Remoting::IObjectManager> ptrOM = m_ptrChannel->GetObjectManager();

	// Get the remote instance IObjectFactory
	IObject* pObjF = NULL;
	ptrOM->GetRemoteInstance(oid,flags,OMEGA_GUIDOF(Activation::IObjectFactory),pObjF);
	ObjectPtr<Activation::IObjectFactory> ptrOF = static_cast<Activation::IObjectFactory*>(pObjF);
	
	// Call CreateInstance
	pObject = NULL;
	ptrOF->CreateInstance(pOuter,iid,pObject);
}

// {3BE419D7-52D9-4873-95E7-836D33523C51}
OMEGA_DEFINE_OID(Compartment,OID_Compartment,"{3BE419D7-52D9-4873-95E7-836D33523C51}");

void OOCore::CompartmentFactory::CreateInstance(IObject* pOuter, const guid_t& iid, IObject*& pObject)
{
	// Compartments are not supported in the OOSvrUser process!
	if (OOCore::HostedByOOServer())
		OMEGA_THROW("Compartments are not supported by the user service");

	// Create a new compartment and get the channel to it...
	ObjectPtr<ObjectImpl<OOCore::ComptChannel> > ptrChannel = OOCore::UserSession::create_compartment();

	// Create a CompartmentImpl
	if (!pOuter)
	{
		ObjectPtr<ObjectImpl<OOCore::CompartmentImpl> > ptrCompt = ObjectImpl<OOCore::CompartmentImpl>::CreateInstance();
		ptrCompt->init(ptrChannel);

		pObject = ptrCompt->QueryInterface(iid);
	}
	else
	{
		ObjectPtr<AggregatedObjectImpl<OOCore::CompartmentImpl> > ptrCompt = AggregatedObjectImpl<OOCore::CompartmentImpl>::CreateInstance(pOuter);
		ptrCompt->ContainedObject()->init(ptrChannel);

		if (iid == OMEGA_GUIDOF(IObject))
			pObject = ptrCompt.Detach();
		else
			pObject = ptrCompt->QueryInterface(iid);
	}
}
