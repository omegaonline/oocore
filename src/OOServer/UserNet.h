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
	class Manager;

	class RemoteChannel : 
		public OTL::ObjectBase,
		public Omega::Remoting::IChannelSink
	{
	public:
		RemoteChannel();

		OTL::ObjectPtr<OTL::ObjectImpl<Channel> > client_init(Manager* pManager, Omega::Remoting::IEndpoint* pEndpoint, const Omega::string_t& strEndpoint, Omega::uint32_t channel_id);
		void server_init(Manager* pManager, Omega::Remoting::IChannelSink* pSink, const Omega::guid_t& message_oid, Omega::uint32_t channel_id);
		void send_away(const ACE_InputCDR& msg, ACE_CDR::ULong src_channel_id, ACE_CDR::ULong dest_channel_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs, ACE_CDR::UShort dest_thread_id, ACE_CDR::UShort src_thread_id, ACE_CDR::UShort flags, ACE_CDR::ULong seq_no);
		void channel_closed(Omega::uint32_t channel_id);
				
		BEGIN_INTERFACE_MAP(RemoteChannel)
			INTERFACE_ENTRY(Omega::Remoting::IChannelSink)
		END_INTERFACE_MAP()

	private:
		ACE_Thread_Mutex                                                    m_lock;
		Manager*                                                            m_pManager;
		Omega::uint32_t                                                     m_channel_id;
		OTL::ObjectPtr<Omega::Remoting::IChannelSink>                       m_ptrUpstream;
		Omega::guid_t                                                       m_message_oid;
		ACE_CDR::ULong                                                      m_nNextChannelId;
		std::map<ACE_CDR::ULong,ACE_CDR::ULong>                             m_mapChannelIds;
		std::map<ACE_CDR::ULong,OTL::ObjectPtr<OTL::ObjectImpl<Channel> > > m_mapChannels;

		OTL::ObjectPtr<OTL::ObjectImpl<Channel> > create_channel(ACE_CDR::ULong channel_id);
		OTL::ObjectPtr<Omega::Remoting::IObjectManager> create_object_manager(ACE_CDR::ULong channel_id);
		void process_here_i(ACE_InputCDR& input);
		void send_away_i(Omega::Remoting::IMessage* pPayload, ACE_CDR::ULong src_channel_id, ACE_CDR::ULong dest_channel_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs, ACE_CDR::UShort dest_thread_id, ACE_CDR::UShort src_thread_id, ACE_CDR::UShort flags, ACE_CDR::ULong seq_no);
		void do_channel_closed_i(Omega::uint32_t channel_id);
		
		static void do_channel_closed(void* pParam, ACE_InputCDR& input);
		static void process_here(void* pParams, ACE_InputCDR& input);
		
	// IChannelSink members
	public:
		void Send(Omega::TypeInfo::MethodAttributes_t attribs, Omega::Remoting::IMessage* pMsg, Omega::uint32_t timeout);
		void Close();
	};
}

#endif // OOSERVER_USER_NET_H_INCLUDED_
