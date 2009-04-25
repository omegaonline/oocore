///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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

#ifndef OOSERVER_USER_MANAGER_H_INCLUDED_
#define OOSERVER_USER_MANAGER_H_INCLUDED_

#include "MessageConnection.h"
#include "UserAcceptor.h"
#include "Protocol.h"
#include "Channel.h"
#include "UserNet.h"

namespace User
{
	typedef OOBase::Singleton<OOSvrBase::Proactor> Proactor;

	class Manager : public Root::MessageHandler
	{
	public:
		static int run(const std::string& strPipe);

		static Omega::Remoting::IChannel* open_remote_channel(const Omega::string_t& strEndpoint);
		static Omega::Remoting::IChannelSink* open_server_sink(const Omega::guid_t& message_oid, Omega::Remoting::IChannelSink* pSink);
		static OTL::ObjectPtr<OTL::ObjectImpl<Channel> > create_channel(Omega::uint32_t src_channel_id, const Omega::guid_t& message_oid);

		OOBase::SmartPtr<OOBase::CDRStream> sendrecv_root(const OOBase::CDRStream& request, Omega::TypeInfo::MethodAttributes_t attribs);

		bool on_accept(OOBase::Socket* sock);

	private:
		friend class OOBase::Singleton<Manager>;
		typedef OOBase::Singleton<Manager> USER_MANAGER;

		static const Omega::uint32_t m_root_channel = 0x80000000;

		Manager();
		virtual ~Manager();

		Manager(const Manager&) : Root::MessageHandler() {}
		Manager& operator = (const Manager&) { return *this; }

		OOBase::RWMutex                                                      m_lock;
		Omega::uint32_t                                                      m_nIPSCookie;
		bool                                                                 m_bIsSandbox;
		Acceptor                                                             m_acceptor;
		std::map<Omega::uint32_t,OTL::ObjectPtr<OTL::ObjectImpl<Channel> > > m_mapChannels;

		virtual bool on_channel_open(Omega::uint32_t channel);
		virtual Root::MessageHandler::io_result::type route_off(OOBase::CDRStream& msg, Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, Omega::uint16_t flags, Omega::uint32_t seq_no);
		virtual void on_channel_closed(Omega::uint32_t channel);

		int run_i(const std::string& strPipe);
		bool init(const std::string& strPipe);
		bool bootstrap(Omega::uint32_t sandbox_channel);
		static void wait_for_quit();
		static void quit();

		OTL::ObjectPtr<OTL::ObjectImpl<Channel> > create_channel_i(Omega::uint32_t src_channel_id, const Omega::guid_t& message_oid);
		OTL::ObjectPtr<Omega::Remoting::IObjectManager> create_object_manager(Omega::uint32_t src_channel_id, const Omega::guid_t& message_oid);
		void process_request(OOBase::CDRStream& request, Omega::uint32_t seq_no, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs);
		void process_user_request(const OOBase::CDRStream& input, Omega::uint32_t seq_no, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs);
		void process_root_request(OOBase::CDRStream& input, Omega::uint32_t seq_no, Omega::uint16_t src_thread_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs);

		// Remote channel handling
		OOBase::RWMutex m_remote_lock;
		Omega::uint32_t m_nNextRemoteChannel;
		struct RemoteChannelEntry
		{
			Omega::string_t strEndpoint;
			OTL::ObjectPtr<OTL::ObjectImpl<RemoteChannel> > ptrRemoteChannel;
		};
		std::map<Omega::uint32_t,RemoteChannelEntry>                         m_mapRemoteChannelIds;
		std::map<Omega::string_t,OTL::ObjectPtr<Omega::Remoting::IChannel> > m_mapRemoteChannels;

		Omega::Remoting::IChannel* open_remote_channel_i(const Omega::string_t& strEndpoint);
		Omega::Remoting::IChannelSink* open_server_sink_i(const Omega::guid_t& message_oid, Omega::Remoting::IChannelSink* pSink);
		void close_all_remotes();
		void local_channel_closed(Omega::uint32_t channel_id);
	};
}

#endif // OOSERVER_USER_MANAGER_H_INCLUDED_

