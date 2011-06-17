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

OOCore::Compartment::ComptState::ComptState(Compartment* cmpt, uint32_t* timeout) : m_cmpt(cmpt)
{
	m_prev_id = m_cmpt->m_pSession->update_state(m_cmpt->m_id,timeout);
}

OOCore::Compartment::ComptState::~ComptState()
{
	m_cmpt->m_pSession->update_state(m_prev_id,0);
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

	OTL::ObjectPtr<OTL::ObjectImpl<ComptChannel> > ptrCompt;
	while (m_mapCompartments.pop(NULL,&ptrCompt))
	{
		guard.release();

		ptrCompt->shutdown();

		guard.acquire();
	}		
}

void OOCore::Compartment::process_compartment_close()
{
	// Switch state...
	ComptState compt_state(this);

	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	ObjectPtr<ObjectImpl<ComptChannel> > ptrCompt;
	m_mapCompartments.erase(compt_state.id(),&ptrCompt);
		
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
		m_mapChannels.erase(channel_id);
	
	return false;
}

ObjectPtr<Remoting::IObjectManager> OOCore::Compartment::get_channel_om(uint32_t src_channel_id)
{
	ObjectPtr<ObjectImpl<OOCore::Channel> > ptrChannel = create_channel(src_channel_id,guid_t::Null());

	return ptrChannel->GetObjectManager();
}

ObjectPtr<ObjectImpl<OOCore::Channel> > OOCore::Compartment::create_channel(uint32_t src_channel_id, const guid_t& message_oid)
{
	// Lookup existing..
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	ChannelInfo info;
	if (m_mapChannels.find(src_channel_id,info))
	{
		assert(info.m_bOpen);
		return info.m_ptrChannel;
	}
	
	read_guard.release();

	// Create a new OM
	ObjectPtr<ObjectImpl<StdObjectManager> > ptrOM = ObjectImpl<StdObjectManager>::CreateInstancePtr();

	// Create a new channel
	ObjectPtr<ObjectImpl<Channel> > ptrChannel = ObjectImpl<Channel>::CreateInstancePtr();
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
		
	return info.m_ptrChannel;
}

void OOCore::Compartment::process_request(Message* pMsg, const OOBase::timeval_t& deadline)
{
	assert(pMsg->m_dest_cmpt_id == m_id);

	// Find and/or create the object manager associated with src_channel_id
	ObjectPtr<Remoting::IObjectManager> ptrOM = get_channel_om(pMsg->m_src_channel_id);

	// QI for IMarshaller
	ObjectPtr<Remoting::IMarshaller> ptrMarshaller(ptrOM);
	if (!ptrMarshaller)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller));

	// Wrap up the request
	ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrEnvelope;
	ptrEnvelope = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();
	ptrEnvelope->init(pMsg->m_payload);

	// Unpack the payload
	ObjectPtr<Remoting::IMessage> ptrRequest = ptrMarshaller.UnmarshalInterface<Remoting::IMessage>(L"payload",ptrEnvelope);

	// Check timeout - at the last possible moment...
	uint32_t timeout = 0;
	if (deadline != OOBase::timeval_t::MaxTime)
	{
		OOBase::timeval_t now = OOBase::timeval_t::gettimeofday();
		if (deadline <= now)
			return;

		timeout = (deadline - now).msec();
	}

	// Make the call
	ObjectPtr<Remoting::IMessage> ptrResult;
	ptrResult.Attach(ptrOM->Invoke(ptrRequest,timeout));

	if (!(pMsg->m_attribs & TypeInfo::Asynchronous))
	{
		// Wrap the response...
		ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrResponse = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();
		ptrMarshaller->MarshalInterface(L"payload",ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);

		// Send it back...
		try
		{
			m_pSession->send_response(pMsg->m_dest_cmpt_id,pMsg->m_seq_no,pMsg->m_src_channel_id,pMsg->m_src_thread_id,ptrResponse->GetCDRStream(),deadline);
		}
		catch (...)
		{
			ptrMarshaller->ReleaseMarshalData(L"payload",ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);
			throw;
		}
	}
}

ObjectPtr<ObjectImpl<OOCore::ComptChannel> > OOCore::Compartment::create_compartment_channel(uint16_t compartment_id, const guid_t& message_oid)
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

		// Create a new OM
		ObjectPtr<ObjectImpl<StdObjectManager> > ptrOM = ObjectImpl<StdObjectManager>::CreateInstancePtr();

		// Create a new channel
		ptrChannel = ObjectImpl<ComptChannel>::CreateInstancePtr();

		ptrChannel->init(ptrCompt,compartment_id | m_pSession->get_channel_id(),ptrOM,message_oid);

		// And add to the map
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		int err = m_mapCompartments.insert(compartment_id,ptrChannel);
		if (err == EEXIST)
			m_mapCompartments.find(compartment_id,ptrChannel);
		else if (err != 0)
			OMEGA_THROW(err);
	}

	return ptrChannel;
}

IException* OOCore::Compartment::compartment_message(Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t timeout)
{
	// Switch state...
	ComptState compt_state(this);
	
	// Find and/or create the object manager associated with src_channel_id
	ObjectPtr<ObjectImpl<ComptChannel> > ptrChannel = create_compartment_channel(compt_state.id(),guid_t::Null());
	ObjectPtr<Remoting::IObjectManager> ptrOM = ptrChannel->GetObjectManager();
	
	// Make the call
	IException* pRet = 0;
	try
	{
		pRecv = ptrOM->Invoke(pSend,timeout);
	}
	catch (IException* pE)
	{
		pRet = pE;
	}

	return pRet;
}

void OOCore::ComptChannel::init(OOBase::SmartPtr<Compartment> ptrCompt, Omega::uint32_t channel_id, Remoting::IObjectManager* pOM, const guid_t& message_oid)
{
	ChannelBase::init(channel_id,Remoting::Compartment,pOM,message_oid);

	m_ptrCompt = ptrCompt;
}

void OOCore::ComptChannel::close_compartment()
{
	// Tell the other end to go...
	m_ptrCompt->shutdown();
}

void OOCore::ComptChannel::shutdown()
{
	m_ptrCompt->process_compartment_close();

	disconnect();
}

Omega::bool_t OOCore::ComptChannel::IsConnected()
{
	return true;
}

IException* OOCore::ComptChannel::SendAndReceive(TypeInfo::MethodAttributes_t /*attribs*/, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t timeout)
{
	return m_ptrCompt->compartment_message(pSend,pRecv,timeout);
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
	m_ptrChannel->close_compartment();

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
	IObject* pObjF = 0;
	ptrOM->GetRemoteInstance(oid,flags,OMEGA_GUIDOF(Activation::IObjectFactory),pObjF);

	ObjectPtr<Activation::IObjectFactory> ptrOF;
	ptrOF.Attach(static_cast<Activation::IObjectFactory*>(pObjF));
	
	// Call CreateInstance
	pObject = 0;
	ptrOF->CreateInstance(pOuter,iid,pObject);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Compartment::ICompartment*,OOCore_ICompartment_Create,0,())
{
	// Compartments are not supported in the OOSvrUser process!
	assert(!OOCore::HostedByOOServer());

	// Create a new compartment and get the channel to it...
	ObjectPtr<ObjectImpl<OOCore::ComptChannel> > ptrChannel = OOCore::UserSession::create_compartment();

	// Create a CompartmentImpl
	ObjectPtr<ObjectImpl<OOCore::CompartmentImpl> > ptrCompt = ObjectImpl<OOCore::CompartmentImpl>::CreateInstancePtr();
	ptrCompt->init(ptrChannel);

	return ptrCompt.AddRef();
}
