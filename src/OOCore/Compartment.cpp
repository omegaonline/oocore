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

using namespace Omega;
using namespace OTL;

OOCore::Compartment::Compartment(UserSession* pSession, uint16_t id) :
		m_pSession(pSession),
		m_id(id)
{
}

void OOCore::Compartment::close()
{
	// Close all open OM's
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		for (std::map<uint32_t,ObjectPtr<ObjectImpl<Channel> > >::iterator j=m_mapChannels.begin(); j!=m_mapChannels.end(); ++j)
		{
			j->second->disconnect();
		}
		m_mapChannels.clear();

		for (std::map<uint16_t,ObjectPtr<ObjectImpl<ComptChannel> > >::iterator i=m_mapCompartments.begin(); i!=m_mapCompartments.end(); ++i)
		{
			i->second->disconnect();
		}
		m_mapCompartments.clear();
	}
	catch (...)
	{}
}

bool OOCore::Compartment::process_channel_close(uint32_t closed_channel_id)
{
	// Close the corresponding Object Manager
	bool bRet = false;
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		for (std::map<uint32_t,ChannelInfo>::iterator i=m_mapChannels.begin(); i!=m_mapChannels.end();++i)
		{
			if (i->first == closed_channel_id)
			{
				// Close if its an exact match
				i->second.m_bOpen = false;
				bRet = true;
			}
			else if (!(closed_channel_id & 0xFFF) && (i->first & 0xFFFFF000) == closed_channel_id)
			{
				// Close all compartments on the channel if 0 cmpt closes
				i->second.m_bOpen = false;
				bRet = true;
			}
		}
	}
	catch (...)
	{}

	return bRet;
}

bool OOCore::Compartment::is_channel_open(uint32_t channel_id)
{
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	std::map<uint32_t,ChannelInfo>::const_iterator i=m_mapChannels.find(channel_id);
	if (i == m_mapChannels.end())
		return false;

	if (i->second.m_bOpen)
		return true;

	read_guard.release();

	// Now remove the closed channel
	OOBase::Guard<OOBase::RWMutex> write_guard(m_lock);

	std::map<uint32_t,ChannelInfo>::iterator j=m_mapChannels.find(channel_id);

	if (j != m_mapChannels.end() && !j->second.m_bOpen)
		m_mapChannels.erase(j);
	
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

	std::map<uint32_t,ChannelInfo>::const_iterator i=m_mapChannels.find(src_channel_id);
	if (i != m_mapChannels.end())
	{
		assert(i->second.m_bOpen);
		return i->second.m_ptrChannel;
	}
	
	read_guard.release();

	// Create a new OM
	ObjectPtr<ObjectImpl<StdObjectManager> > ptrOM = ObjectImpl<StdObjectManager>::CreateInstancePtr();

	// Create a new channel
	ObjectPtr<ObjectImpl<Channel> > ptrChannel = ObjectImpl<Channel>::CreateInstancePtr();
	ptrChannel->init(m_pSession,src_channel_id,ptrOM,message_oid);

	// And add to the map
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	ChannelInfo info;
	info.m_bOpen = true;
	info.m_ptrChannel = ptrChannel;

	std::pair<std::map<uint32_t,ChannelInfo>::iterator,bool> p = m_mapChannels.insert(std::map<uint32_t,ChannelInfo>::value_type(src_channel_id,info));
	if (!p.second)
		ptrChannel = p.first->second.m_ptrChannel;

	return ptrChannel;
}

void OOCore::Compartment::process_request(const Message* pMsg, const OOBase::timeval_t& deadline)
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

ObjectPtr<ObjectImpl<OOCore::ComptChannel> > OOCore::Compartment::create_compartment(uint16_t compartment_id, const guid_t& message_oid)
{
	// Lookup existing..
	ObjectPtr<ObjectImpl<ComptChannel> > ptrChannel;

	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	std::map<uint16_t,ObjectPtr<ObjectImpl<ComptChannel> > >::const_iterator i=m_mapCompartments.find(compartment_id);
	if (i != m_mapCompartments.end())
		ptrChannel = i->second;

	read_guard.release();

	if (!ptrChannel)
	{
		// Get the compartment
		OOBase::SmartPtr<OOCore::Compartment> ptrCompt = m_pSession->get_compartment(compartment_id);
		if (!ptrCompt)
			throw Remoting::IChannelClosedException::Create();

		// Create a new OM
		ObjectPtr<ObjectImpl<StdObjectManager> > ptrOM = ObjectImpl<StdObjectManager>::CreateInstancePtr();

		// Create a new channel
		ptrChannel = ObjectImpl<ComptChannel>::CreateInstancePtr();

		ptrChannel->init(ptrCompt,m_id | m_pSession->get_channel_id(),ptrOM,message_oid);

		// And add to the map
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		std::pair<std::map<uint16_t,ObjectPtr<ObjectImpl<ComptChannel> > >::iterator,bool> p = m_mapCompartments.insert(std::map<uint16_t,ObjectPtr<ObjectImpl<ComptChannel> > >::value_type(compartment_id,ptrChannel));
		if (!p.second)
			ptrChannel = p.first->second;
	}

	return ptrChannel;
}

IException* OOCore::Compartment::compartment_message(uint16_t cmpt_id, TypeInfo::MethodAttributes_t /*attribs*/, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t timeout)
{
	// Find and/or create the object manager associated with src_channel_id
	ObjectPtr<ObjectImpl<ComptChannel> > ptrChannel = create_compartment(cmpt_id,guid_t::Null());
	ObjectPtr<Remoting::IObjectManager> ptrOM = ptrChannel->GetObjectManager();

	// Update session state and timeout
	uint16_t old_id = m_pSession->update_state(m_id,&timeout);

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

	m_pSession->update_state(old_id,0);

	return pRet;
}

void OOCore::ComptChannel::init(OOBase::SmartPtr<Compartment> ptrCompt, Omega::uint32_t channel_id, Remoting::IObjectManager* pOM, const guid_t& message_oid)
{
	ChannelBase::init(channel_id,Remoting::Compartment,pOM,message_oid);

	m_ptrCompt = ptrCompt;
}

Omega::bool_t OOCore::ComptChannel::IsConnected()
{
	return true;
}

IException* OOCore::ComptChannel::SendAndReceive(TypeInfo::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t timeout)
{
	return m_ptrCompt->compartment_message(static_cast<uint16_t>(m_channel_id & 0xFFF),attribs,pSend,pRecv,timeout);
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
	// Propogate close message upstream and out to other compartments...
	delete this;
	void* TODO;
}

void OOCore::CompartmentImpl::init(ObjectPtr<ObjectImpl<OOCore::ComptChannel> > ptrChannel)
{
	m_ptrChannel = ptrChannel;
}

void OOCore::CompartmentImpl::CreateInstance(const any_t& oid, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid, IObject*& pObject)
{
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
