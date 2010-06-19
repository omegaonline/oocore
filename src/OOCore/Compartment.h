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

#ifndef OOCORE_APARTMENT_IMPL_H_INCLUDED_
#define OOCORE_APARTMENT_IMPL_H_INCLUDED_

#include "../include/Omega/Compartment.h"

#include "Channel.h"

namespace OOCore
{
	struct Message;
	class UserSession;
	class Compartment;

	class ComptChannel :
			public ChannelBase
	{
	public:
		void init(OOBase::SmartPtr<Compartment> ptrCompt, Omega::uint32_t channel_id, Omega::Remoting::IObjectManager* pOM, const Omega::guid_t& message_oid);
		
		BEGIN_INTERFACE_MAP(ComptChannel)
			INTERFACE_ENTRY_CHAIN(ChannelBase)
		END_INTERFACE_MAP()

	private:
		OOBase::SmartPtr<Compartment> m_ptrCompt;

	public:
		Omega::bool_t IsConnected();
		Omega::IException* SendAndReceive(Omega::TypeInfo::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv, Omega::uint32_t timeout);
	};

	class Compartment
	{
	public:
		Compartment(UserSession* pSession, Omega::uint16_t id);

		void close();
		
		void process_channel_close(Omega::uint32_t closed_channel_id);
		bool is_channel_open(Omega::uint32_t channel_id);

		OTL::ObjectPtr<Omega::Remoting::IObjectManager> get_channel_om(Omega::uint32_t src_channel_id);
		OTL::ObjectPtr<OTL::ObjectImpl<Channel> > create_channel(Omega::uint32_t src_channel_id, const Omega::guid_t& message_oid);
		void process_request(const Message* pMsg, const OOBase::timeval_t& deadline);

		OTL::ObjectPtr<OTL::ObjectImpl<ComptChannel> > create_compartment(Omega::uint16_t compartment_id, const Omega::guid_t& message_oid);
		Omega::IException* compartment_message(Omega::uint16_t cmpt_id, Omega::TypeInfo::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv, Omega::uint32_t timeout);

	private:
		OOBase::RWMutex m_lock;
		UserSession*    m_pSession;
		Omega::uint16_t m_id;

		std::map<Omega::uint32_t,OTL::ObjectPtr<OTL::ObjectImpl<Channel> > >    m_mapChannels;
		std::map<Omega::uint16_t,OTL::ObjectPtr<OTL::ObjectImpl<ComptChannel> > > m_mapCompartments;
	};
}

#endif // OOCORE_APARTMENT_IMPL_H_INCLUDED_
