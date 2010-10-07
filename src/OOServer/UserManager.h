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

int main(int, char**);

namespace User
{
	typedef OOBase::Singleton<OOSvrBase::Proactor,User::Module> Proactor;

	class Manager :
			public OOServer::MessageHandler,
			public OOSvrBase::Server
	{
		// main() has full access...
		friend int ::main(int, char**);

	public:
		static Omega::Remoting::IChannel* open_remote_channel(const Omega::string_t& strEndpoint);
		static Omega::Remoting::IChannelSink* open_server_sink(const Omega::guid_t& message_oid, Omega::Remoting::IChannelSink* pSink);
		static OTL::ObjectPtr<OTL::ObjectImpl<Channel> > create_channel(Omega::uint32_t src_channel_id, const Omega::guid_t& message_oid);

		OOBase::SmartPtr<OOBase::CDRStream> sendrecv_root(const OOBase::CDRStream& request, Omega::TypeInfo::MethodAttributes_t attribs);

		bool on_accept(OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket>& ptrSocket);
		void close_socket(Omega::uint32_t id);

	private:
		static Manager* s_instance; //  This is a poor-mans singleton
		static const Omega::uint32_t m_root_channel = 0x80000000;

		Manager();
		virtual ~Manager();

		Manager(const Manager&);
		Manager& operator = (const Manager&);

		OOBase::RWMutex                                                      m_lock;
		Omega::uint32_t                                                      m_nIPSCookie;
		bool                                                                 m_bIsSandbox;
		Acceptor                                                             m_acceptor;
		std::map<Omega::uint32_t,OTL::ObjectPtr<OTL::ObjectImpl<Channel> > > m_mapChannels;

		virtual OOServer::MessageHandler::io_result::type route_off(OOBase::CDRStream& msg, Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, Omega::uint16_t flags, Omega::uint32_t seq_no);
		virtual void on_channel_closed(Omega::uint32_t channel);

		void run();
		bool fork_slave(const std::string& strPipe);
		bool session_launch(const std::string& strPipe);

		bool handshake_root(OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket>& local_socket, const std::string& strPipe);
		static void do_bootstrap(void* pParams, OOBase::CDRStream& input);
		bool bootstrap(Omega::uint32_t sandbox_channel);

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

		// Service handling
		OOBase::RWMutex m_service_lock;
		Omega::uint32_t m_nNextService;

		struct Service
		{
			std::string                             strKey;
			OTL::ObjectPtr<Omega::System::IService> ptrService;
		};
		std::map<Omega::uint32_t,Service>               m_mapServices;
		std::map<Omega::uint32_t,OOSvrBase::IOHandler*> m_mapSockets;

		bool start_services();
		void start_service(const std::string& strKey, const std::string& strOid);
		OTL::ObjectPtr<Omega::Registry::IKey> get_service_key(const std::string& strKey);
		void listen_service_socket(const std::string& strKey, Omega::uint32_t nServiceId, OTL::ObjectPtr<Omega::System::INetworkService> ptrNetService);
		void stop_services();

		void on_socket_accept(OOBase::CDRStream& request, OOBase::CDRStream& response);
		void on_socket_recv(OOBase::CDRStream& request);
		void on_socket_sent(OOBase::CDRStream& request);
		void on_socket_close(OOBase::CDRStream& request);
	};
}

#endif // OOSERVER_USER_MANAGER_H_INCLUDED_
