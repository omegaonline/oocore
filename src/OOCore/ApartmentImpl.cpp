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

OMEGA_DEFINE_OID(OOCore,OID_StdApartment,"{6654B003-44F1-497a-B539-80B5FCED73BC}");

OOCore::Apartment::Apartment(UserSession* pSession, uint16_t id) :
	m_pSession(pSession),
	m_id(id)
{
}

void OOCore::Apartment::close()
{
	// Close all open OM's
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		for (std::map<uint32_t,ObjectPtr<ObjectImpl<Channel> > >::iterator j=m_mapChannels.begin();j!=m_mapChannels.end();++j)
		{
			j->second->disconnect();
		}
		m_mapChannels.clear();

		for (std::map<uint16_t,OTL::ObjectPtr<OTL::ObjectImpl<AptChannel> > >::iterator i=m_mapApartments.begin();i!=m_mapApartments.end();++i)
		{
			i->second->disconnect();
		}
		m_mapApartments.clear();
	}
	catch (std::exception&)
	{}
}

void OOCore::Apartment::process_channel_close(uint32_t closed_channel_id)
{
	// Close the corresponding Object Manager
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		std::list<ObjectPtr<ObjectImpl<Channel> > > listChannels;

		for (std::map<uint32_t,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.begin();i!=m_mapChannels.end();)
		{
			bool bErase = false;
			if (i->first == closed_channel_id)
			{
				// Close if its an exact match
				bErase = true;
			}
			else if ((i->first & 0xFFFFF000) == closed_channel_id)
			{
				// Close all apartments on the channel
				bErase = true;
			}
			
			if (bErase)
			{
				listChannels.push_back(i->second);
				m_mapChannels.erase(i++);
			}
			else
				++i;
		}

		guard.release();

		for (std::list<ObjectPtr<ObjectImpl<Channel> > >::iterator i=listChannels.begin();i!=listChannels.end();++i)
		{
			(*i)->disconnect();
		}
	}
	catch (std::exception&)
	{}
}

bool OOCore::Apartment::is_channel_open(uint32_t channel_id)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	try
	{
		std::map<uint32_t,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.find(channel_id);
		return (i != m_mapChannels.end());
	}
	catch (std::exception&)
	{
		return false;
	}
}

ObjectPtr<Remoting::IObjectManager> OOCore::Apartment::get_channel_om(uint32_t src_channel_id)
{
	ObjectPtr<ObjectImpl<OOCore::Channel> > ptrChannel = create_channel(src_channel_id,guid_t::Null());
	
	return ptrChannel->GetObjectManager();
}

ObjectPtr<ObjectImpl<OOCore::Channel> > OOCore::Apartment::create_channel(uint32_t src_channel_id, const guid_t& message_oid)
{
	// Lookup existing..
	try
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		std::map<uint32_t,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.find(src_channel_id);
		if (i != m_mapChannels.end())
			return i->second;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	// Create a new OM
	ObjectPtr<ObjectImpl<StdObjectManager> > ptrOM = ObjectImpl<StdObjectManager>::CreateInstancePtr();
		
	// Create a new channel
	ObjectPtr<ObjectImpl<Channel> > ptrChannel = ObjectImpl<Channel>::CreateInstancePtr();
	ptrChannel->init(m_pSession,m_id,src_channel_id,ptrOM,message_oid);

	// And add to the map
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		std::pair<std::map<uint32_t,ObjectPtr<ObjectImpl<Channel> > >::iterator,bool> p = m_mapChannels.insert(std::map<uint32_t,ObjectPtr<ObjectImpl<Channel> > >::value_type(src_channel_id,ptrChannel));
		if (!p.second)
			ptrChannel = p.first->second;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	return ptrChannel;
}

void OOCore::Apartment::process_request(const Message* pMsg, const OOBase::timeval_t& deadline)
{
	// Find and/or create the object manager associated with src_channel_id
	ObjectPtr<Remoting::IObjectManager> ptrOM = get_channel_om(pMsg->m_src_channel_id);

	// QI for IMarshaller
	ObjectPtr<Remoting::IMarshaller> ptrMarshaller(ptrOM);
	if (!ptrMarshaller)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller),OMEGA_SOURCE_INFO);
	
	// Wrap up the request
	ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrEnvelope;
	ptrEnvelope = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();
	ptrEnvelope->init(pMsg->m_payload);

	// Unpack the payload
	ObjectPtr<Remoting::IMessage> ptrRequest = ptrMarshaller.UnmarshalInterface<Remoting::IMessage>(L"payload",ptrEnvelope);
	
	// Check timeout
	uint32_t timeout = 0;
	if (deadline != OOBase::timeval_t::MaxTime)
	{
		OOBase::timeval_t now = OOBase::gettimeofday();
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
			m_pSession->send_response(m_id,pMsg->m_seq_no,pMsg->m_src_channel_id,pMsg->m_src_thread_id,ptrResponse->GetCDRStream(),deadline);
		}
		catch (...)
		{
			ptrMarshaller->ReleaseMarshalData(L"payload",ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);
			throw;
		}
	}
}

ObjectPtr<Remoting::IObjectManager> OOCore::Apartment::get_apartment_om(uint16_t apartment_id)
{
	ObjectPtr<ObjectImpl<AptChannel> > ptrChannel = create_apartment(apartment_id,guid_t::Null());
	
	return ptrChannel->GetObjectManager();
}

ObjectPtr<ObjectImpl<OOCore::AptChannel> > OOCore::Apartment::create_apartment(uint16_t apartment_id, const guid_t& message_oid)
{
	// Lookup existing..
	ObjectPtr<ObjectImpl<AptChannel> > ptrChannel;
	try
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		std::map<uint16_t,OTL::ObjectPtr<OTL::ObjectImpl<AptChannel> > >::iterator i=m_mapApartments.find(apartment_id);
		if (i != m_mapApartments.end())
			ptrChannel = i->second;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	if (!ptrChannel)
	{
		// Create a new OM
		ObjectPtr<ObjectImpl<StdObjectManager> > ptrOM = ObjectImpl<StdObjectManager>::CreateInstancePtr();
		
		// Create a new channel
		ptrChannel = ObjectImpl<AptChannel>::CreateInstancePtr();
		ptrChannel->init(m_pSession->get_apartment(apartment_id),m_id | m_pSession->get_channel_id(),ptrOM,message_oid);

		// And add to the map
		try
		{
			OOBase::Guard<OOBase::RWMutex> guard(m_lock);

			std::pair<std::map<uint16_t,OTL::ObjectPtr<OTL::ObjectImpl<AptChannel> > >::iterator,bool> p = m_mapApartments.insert(std::map<uint16_t,OTL::ObjectPtr<OTL::ObjectImpl<AptChannel> > >::value_type(apartment_id,ptrChannel));
			if (!p.second)
				ptrChannel = p.first->second;
		}
		catch (std::exception& e)
		{
			OMEGA_THROW(e);
		}
	}

	return ptrChannel;
}

IException* OOCore::Apartment::apartment_message(uint16_t apt_id, TypeInfo::MethodAttributes_t /*attribs*/, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t timeout)
{
	// Find and/or create the object manager associated with src_channel_id
	ObjectPtr<Remoting::IObjectManager> ptrOM = get_apartment_om(apt_id);

	// Update session state and timeout
	uint16_t old_id = 0;
	old_id = m_pSession->update_state(apt_id,&timeout);
	
	// Make the call
	try
	{
		pRecv = ptrOM->Invoke(pSend,timeout);
	}
	catch (IException* pE)
	{
		// Reset state
		m_pSession->update_state(old_id,0);
		return pE;
	}

	m_pSession->update_state(old_id,0);
	return 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Apartment::IApartment*,OOCore_IApartment_Create,0,())
{
	// Apartments are not supported in the OOSvrUser process!
	assert(!OOCore::HostedByOOServer());

	// Create a new apartment
	return OOCore::UserSession::create_apartment();
}

OOCore::ApartmentImpl::ApartmentImpl() : 
	m_id(0)
{
	m_id = UserSession::get_current_apartment();
}

OOCore::ApartmentImpl::~ApartmentImpl()
{
	if (m_id)
		UserSession::remove_apartment(m_id);
}

void OOCore::ApartmentImpl::CreateInstance(const string_t& strURI, Activation::Flags_t flags, IObject* pOuter, const guid_t& iid, IObject*& pObject)
{
	pObject = Omega::CreateInstance(strURI,flags,pOuter,iid);
}

void OOCore::AptChannel::init(OOBase::SmartPtr<Apartment> ptrApt, Omega::uint32_t channel_id, Remoting::IObjectManager* pOM, const guid_t& message_oid)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	ChannelBase::init(channel_id,Remoting::Apartment,pOM,message_oid);

	m_ptrApt = ptrApt;
}

IException* OOCore::AptChannel::SendAndReceive(TypeInfo::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t timeout)
{
	return m_ptrApt->apartment_message(static_cast<uint16_t>(m_channel_id & 0xFFF),attribs,pSend,pRecv,timeout);
}
