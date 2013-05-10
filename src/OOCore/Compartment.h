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

#ifndef OOCORE_COMPARTMENT_IMPL_H_INCLUDED_
#define OOCORE_COMPARTMENT_IMPL_H_INCLUDED_

#include "../../include/Omega/Compartment.h"

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
		void init(Omega::uint16_t src_compt_id, const OOBase::SmartPtr<Compartment>& ptrCompt, Omega::uint32_t channel_id, Omega::Remoting::IObjectManager* pOM, const Omega::guid_t& message_oid);
		void shutdown();
		
		BEGIN_INTERFACE_MAP(ComptChannel)
			INTERFACE_ENTRY_CHAIN(ChannelBase)
		END_INTERFACE_MAP()

	private:
		Omega::uint16_t               m_src_compt_id;
		OOBase::SmartPtr<Compartment> m_ptrCompt;

	public:
		Omega::bool_t IsConnected();
		Omega::IException* SendAndReceive(Omega::TypeInfo::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv);
	};

	class Compartment :
			public Omega::System::Internal::ThrowingNew,
			public OOBase::NonCopyable
	{
	public:
		Compartment(UserSession* pSession);

		void set_id(Omega::uint16_t id);
		void shutdown();	
		bool process_channel_close(Omega::uint32_t closed_channel_id);
		void process_compartment_close(Omega::uint16_t src_compt_id);
		bool is_channel_open(Omega::uint32_t channel_id);

		Omega::Remoting::IObjectManager* unmarshal_om(Omega::Remoting::IMessage* pMessage);

		Omega::Remoting::IObjectManager* get_channel_om(Omega::uint32_t src_channel_id);
		OTL::ObjectImpl<Channel>* create_channel(Omega::uint32_t src_channel_id, const Omega::guid_t& message_oid, Omega::Remoting::MarshalFlags_t flags);
		void process_request(const Message& msg);

		OTL::ObjectImpl<ComptChannel>* create_compartment_channel(Omega::uint16_t compartment_id, const Omega::guid_t& message_oid);
		Omega::IException* compartment_message(Omega::uint16_t src_compt_id, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv);

	private:
		OOBase::RWMutex m_lock;
		UserSession*    m_pSession;
		Omega::uint16_t m_id;

		struct ComptState : public OOBase::NonCopyable
		{
			ComptState(Compartment* cmpt);
			~ComptState();

		private:
			Compartment* const m_cmpt;
			Omega::uint16_t    m_prev_id;
		};
		friend struct ComptState;

		struct ChannelInfo
		{
			OTL::ObjectPtr<Omega::Remoting::IChannel> m_ptrChannel;
			bool                                      m_bOpen;
		};

		OOBase::HashTable<Omega::uint32_t,ChannelInfo>                                     m_mapChannels;
		OOBase::HashTable<Omega::uint16_t,OTL::ObjectPtr<OTL::ObjectImpl<ComptChannel> > > m_mapCompartments;
		OTL::ObjectPtr<Omega::Activation::IRunningObjectTable>                             m_ptrROT;
	};
	
	class CompartmentFactory : 
		public OTL::ObjectFactoryBase<&Omega::Compartment::OID_Compartment,Omega::Activation::ProcessScope>
	{
	// IObjectFactory members
	public:
		void CreateObject(const Omega::guid_t& iid, Omega::IObject*& pObject);
	};
}

#endif // OOCORE_COMPARTMENT_IMPL_H_INCLUDED_
