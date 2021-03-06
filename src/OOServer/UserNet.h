///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
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

#ifndef OOSERVER_USER_NET_H_INCLUDED_
#define OOSERVER_USER_NET_H_INCLUDED_

namespace User
{
	class RemoteChannel :
			public OTL::ObjectBase,
			public Omega::Remoting::IChannelSink
	{
	public:
		RemoteChannel();

		OTL::ObjectImpl<Channel>* client_init(Omega::Remoting::IEndpoint* pEndpoint, const Omega::string_t& strEndpoint, Omega::uint32_t channel_id);
		void server_init(Omega::Remoting::IChannelSink* pSink, const Omega::guid_t& message_oid, Omega::uint32_t channel_id);
		void send_away(OOBase::CDRStream& msg, Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, OOServer::Message_t::Type type);
		void channel_closed(Omega::uint32_t channel_id);

		BEGIN_INTERFACE_MAP(RemoteChannel)
			INTERFACE_ENTRY(Omega::Remoting::IChannelSink)
		END_INTERFACE_MAP()

	private:
		OOBase::Mutex                                                        m_lock;
		Omega::uint32_t                                                      m_channel_id;
		OTL::ObjectPtr<Omega::Remoting::IChannelSink>                        m_ptrUpstream;
		Omega::guid_t                                                        m_message_oid;
		Omega::uint32_t                                                      m_nNextChannelId;
		OOBase::HashTable<Omega::uint32_t,Omega::uint32_t>                            m_mapChannelIds;
		OOBase::HashTable<Omega::uint32_t,OTL::ObjectPtr<OTL::ObjectImpl<Channel> > > m_mapChannels;

		OTL::ObjectImpl<Channel>* create_channel(Omega::uint32_t channel_id);
		Omega::Remoting::IObjectManager* create_object_manager(Omega::uint32_t channel_id);
		void process_here_i(OOBase::CDRStream& input);
		void send_away_i(Omega::Remoting::IMessage* pPayload, Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, OOServer::Message_t::Type type);
		
		static void process_here(void* pParams, OOBase::CDRStream& input, OOBase::AllocatorInstance&);

	// IChannelSink members
	public:
		void Send(Omega::TypeInfo::MethodAttributes_t attribs, Omega::Remoting::IMessage* pMsg);
		void Close();
	};
}

#endif // OOSERVER_USER_NET_H_INCLUDED_
