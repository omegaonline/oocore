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

#include "./UserSession.h"

using namespace Omega;
using namespace OTL;

OOCore::Apartment::Apartment(UserSession* pSession, ACE_CDR::UShort id) :
	m_pSession(pSession),
	m_id(id)
{
}

void OOCore::Apartment::close()
{
	// Close all open OM's
	try
	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		for (std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator j=m_mapChannels.begin();j!=m_mapChannels.end();++j)
		{
			j->second->disconnect();
		}
		m_mapChannels.clear();
	}
	catch (std::exception&)
	{}
}

void OOCore::Apartment::process_channel_close(ACE_CDR::ULong closed_channel_id)
{
	// Close the corresponding Object Manager
	try
	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		for (std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.begin();i!=m_mapChannels.end();)
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
				i->second->disconnect();
				m_mapChannels.erase(i++);
			}
			else
				++i;
		}
	}
	catch (std::exception&)
	{}
}

bool OOCore::Apartment::is_channel_open(ACE_CDR::ULong channel_id)
{
	ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

	try
	{
		std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.find(channel_id);
		return (i != m_mapChannels.end());
	}
	catch (std::exception&)
	{
		return false;
	}
}

ObjectPtr<Remoting::IObjectManager> OOCore::Apartment::get_channel_om(ACE_CDR::ULong src_channel_id, const guid_t& message_oid)
{
	ObjectPtr<ObjectImpl<OOCore::Channel> > ptrChannel = create_channel(src_channel_id,message_oid);
	ObjectPtr<Remoting::IObjectManager> ptrOM;
	ptrOM.Attach(ptrChannel->GetObjectManager());
	return ptrOM;
}

ObjectPtr<ObjectImpl<OOCore::Channel> > OOCore::Apartment::create_channel(ACE_CDR::ULong src_channel_id, const guid_t& message_oid)
{
	try
	{
		// Lookup existing..
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.find(src_channel_id);
			if (i != m_mapChannels.end())
				return i->second;
		}

		// Create a new channel
		ObjectPtr<ObjectImpl<Channel> > ptrChannel = ObjectImpl<Channel>::CreateInstancePtr();
		ptrChannel->init(m_pSession,m_id,src_channel_id,m_pSession->classify_channel(src_channel_id),message_oid);

		// And add to the map
		OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::pair<std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator,bool> p = m_mapChannels.insert(std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::value_type(src_channel_id,ptrChannel));
		if (!p.second)
		{
			if (p.first->second->GetMarshalFlags() != ptrChannel->GetMarshalFlags())
				OMEGA_THROW(EINVAL);

			ptrChannel = p.first->second;
		}

		return ptrChannel;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void OOCore::Apartment::process_request(const Message* pMsg, const ACE_Time_Value& deadline)
{
	// Find and/or create the object manager associated with src_channel_id
	ObjectPtr<Remoting::IObjectManager> ptrOM = get_channel_om(pMsg->m_src_channel_id,guid_t::Null());
	
	// Wrap up the request
	ObjectPtr<ObjectImpl<OOCore::InputCDR> > ptrEnvelope;
	ptrEnvelope = ObjectImpl<OOCore::InputCDR>::CreateInstancePtr();
	ptrEnvelope->init(*pMsg->m_ptrPayload);

	// Unpack the payload
	IObject* pPayload = 0;
	ptrOM->UnmarshalInterface(L"payload",ptrEnvelope,OMEGA_GUIDOF(Remoting::IMessage),pPayload);
	ObjectPtr<Remoting::IMessage> ptrRequest;
	ptrRequest.Attach(static_cast<Remoting::IMessage*>(pPayload));

	// Check timeout
	uint32_t timeout = 0;
	if (deadline != ACE_Time_Value::max_time)
	{
		ACE_Time_Value now = ACE_OS::gettimeofday();
		if (deadline <= now)
		{
			ACE_OS::last_error(ETIMEDOUT);
			return;
		}
		timeout = (deadline - now).msec();
	}

	// Make the call
	ObjectPtr<Remoting::IMessage> ptrResult;
	ptrResult.Attach(ptrOM->Invoke(ptrRequest,timeout));

	if (!(pMsg->m_attribs & Remoting::Asynchronous))
	{
		// Wrap the response...
		ObjectPtr<ObjectImpl<OOCore::OutputCDR> > ptrResponse = ObjectImpl<OOCore::OutputCDR>::CreateInstancePtr();
		ptrOM->MarshalInterface(L"payload",ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);

		// Send it back...
		const ACE_Message_Block* mb = static_cast<const ACE_Message_Block*>(ptrResponse->GetMessageBlock());
		if (!m_pSession->send_response(m_id,pMsg->m_seq_no,pMsg->m_src_channel_id,pMsg->m_src_thread_id,mb,deadline))
			ptrOM->ReleaseMarshalData(L"payload",ptrResponse,OMEGA_GUIDOF(Remoting::IMessage),ptrResult);
	}
}

namespace OOCore
{
	class ApartmentImpl :
		public ObjectBase,
		public Omega::Apartment::IApartment
	{
	public:
		ApartmentImpl();

		void init(const guid_t& om_oid);

		BEGIN_INTERFACE_MAP(ApartmentImpl)
			INTERFACE_ENTRY(Omega::Apartment::IApartment)
		END_INTERFACE_MAP()

	// IApartment members
	public:
		void CreateInstance(IObject* pOuter, const guid_t& iid, IObject*& pObject);
	};
}

OOCore::ApartmentImpl::ApartmentImpl()
{
}

void OOCore::ApartmentImpl::init(const guid_t& om_oid)
{
	ACE_Refcounted_Auto_Ptr<Apartment,ACE_Thread_Mutex> ptrApt = UserSession::create_apartment();
}

void OOCore::ApartmentImpl::CreateInstance(IObject* pOuter, const guid_t& iid, IObject*& pObject)
{
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Apartment::IApartment*,IApartment_Create,1,((in),const guid_t&,om_oid))
{
	if (OOCore::HostedByOOServer())
		OMEGA_THROW(L"Apartments are not supported in the OOSvrUser process!");

	// Create a new apartment
	ObjectPtr<ObjectImpl<OOCore::ApartmentImpl> > ptrApt = ObjectImpl<OOCore::ApartmentImpl>::CreateInstancePtr();
	ptrApt->init(om_oid);

	// Return it
	return ptrApt.AddRef();
}
