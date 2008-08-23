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

#include "./MessageConnection.h"
#include "./Protocol.h"
#include "./Channel.h"
#include "./UserNet.h"
#include "./UserHttp.h"

namespace User
{
	class Manager : public Root::MessageHandler
	{
	public:
		static int run(const ACE_CString& strPipe);

		static Omega::Remoting::IChannel* open_remote_channel(const Omega::string_t& strEndpoint);
		static Omega::Remoting::IChannelSink* open_server_sink(const Omega::guid_t& message_oid, Omega::Remoting::IChannelSink* pSink);
		static OTL::ObjectPtr<OTL::ObjectImpl<Channel> > create_channel(ACE_CDR::ULong src_channel_id, const Omega::guid_t& message_oid);

		static bool call_async_function(void (*pfnCall)(void*,ACE_InputCDR&), void* pParam, const ACE_Message_Block* mb = 0);

		ACE_InputCDR* sendrecv_root(const ACE_OutputCDR& request, Omega::Remoting::MethodAttributes_t attribs);
		void handle_http_request(HttpConnection* pConn, Omega::uint16_t conn_id);

	private:
		friend class ACE_Singleton<Manager, ACE_Recursive_Thread_Mutex>;
		friend class Root::MessagePipeAsyncAcceptor<Manager>;
		typedef ACE_Singleton<Manager, ACE_Recursive_Thread_Mutex> USER_MANAGER;

		Manager();
		virtual ~Manager();

		Manager(const Manager&) : Root::MessageHandler() {}
		Manager& operator = (const Manager&) { return *this; }

		ACE_RW_Thread_Mutex                                                 m_lock;
		Omega::uint32_t                                                     m_nIPSCookie;
		bool                                                                m_bIsSandbox;
		Root::MessagePipeAsyncAcceptor<Manager>                             m_process_acceptor;
		std::map<ACE_CDR::ULong,OTL::ObjectPtr<OTL::ObjectImpl<Channel> > > m_mapChannels;

		static const ACE_CDR::ULong m_root_channel;

		int on_accept(const ACE_Refcounted_Auto_Ptr<Root::MessagePipe,ACE_Thread_Mutex>& pipe);

		virtual bool on_channel_open(ACE_CDR::ULong channel);
		virtual bool route_off(ACE_InputCDR& msg, ACE_CDR::ULong src_channel_id, ACE_CDR::ULong dest_channel_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs, ACE_CDR::UShort dest_thread_id, ACE_CDR::UShort src_thread_id, ACE_CDR::UShort flags, ACE_CDR::ULong seq_no);
		virtual void on_channel_closed(ACE_CDR::ULong channel);

		int run_event_loop_i(const ACE_CString& strPipe);
		bool init(const ACE_CString& strPipe);
		bool bootstrap(ACE_CDR::ULong sandbox_channel);
		void end();

		OTL::ObjectPtr<OTL::ObjectImpl<Channel> > create_channel_i(ACE_CDR::ULong src_channel_id, const Omega::guid_t& message_oid);
		OTL::ObjectPtr<Omega::Remoting::IObjectManager> create_object_manager(ACE_CDR::ULong src_channel_id, const Omega::guid_t& message_oid);
		void process_request(ACE_InputCDR& request, ACE_CDR::ULong seq_no, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs);
		void process_user_request(const ACE_InputCDR& input, ACE_CDR::ULong seq_no, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs);
		void process_root_request(ACE_InputCDR& input, ACE_CDR::ULong seq_no, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs);

		// Remote channel handling
		ACE_RW_Thread_Mutex m_remote_lock;
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
		void local_channel_closed(ACE_CDR::ULong channel_id);

		// Services
		std::map<Omega::string_t,OTL::ObjectPtr<Omega::System::IService> > m_mapServices;

		bool start_service(const Omega::string_t& strName, const Omega::guid_t& oid);
		void service_start_i();
		void stop_services();
		static void service_start(void* pParam, ACE_InputCDR&);

		// HTTP handling (for sandbox only)
		ACE_RW_Thread_Mutex                                                                  m_http_lock;
		std::map<Omega::uint16_t,ACE_Refcounted_Auto_Ptr<HttpConnection,ACE_Thread_Mutex> >  m_mapHttpConnections;
		std::map<Omega::string_t,OTL::ObjectPtr<Omega::Net::Http::Server::IRequestHandler> > m_mapHttpHandlers;
		void close_all_http();

		void open_http(ACE_InputCDR& request, ACE_OutputCDR& response);
		void recv_http(ACE_InputCDR& request);
		void handle_http_request_i(HttpConnection* pConn, Omega::uint16_t conn_id);
	};
}

#endif // OOSERVER_USER_MANAGER_H_INCLUDED_

