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

/////////////////////////////////////////////////////////////
//
//  ***** THIS IS A SECURE MODULE *****
//
//  It can be run as Administrator/setuid root
//
//  Therefore it needs to be SAFE AS HOUSES!
//
//  Do not include anything unnecessary
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_MSG_CONNECTION_H_INCLUDED_
#define OOSERVER_MSG_CONNECTION_H_INCLUDED_

namespace OOServer
{
	class MessageHandler;

	class MessageConnection : public OOBase::RefCounted
	{
	public:
		MessageConnection(MessageHandler* pHandler, OOBase::RefPtr<OOSvrBase::AsyncLocalSocket>& ptrSocket);
		
		void set_channel_id(Omega::uint32_t channel_id);

		void shutdown();
		int recv();
		int send(OOBase::Buffer* pBuffer1, OOBase::Buffer* pBuffer2);

	private:
		MessageConnection(const MessageConnection&);
		MessageConnection& operator = (const MessageConnection&);
		virtual ~MessageConnection();

		OOBase::SpinLock                            m_lock;
		MessageHandler*                             m_pHandler;
		OOBase::RefPtr<OOSvrBase::AsyncLocalSocket> m_ptrSocket;
		Omega::uint32_t                             m_channel_id;

		static void on_recv1(void* param, OOBase::Buffer* buffer, int err);
		static void on_recv2(void* param, OOBase::Buffer* buffer, int err);
		bool on_recv(OOBase::Buffer* buffer, int err, int part);
		static void on_sent(void* param, int err);
		void on_closed();
	};

	struct Message_t
	{
		enum Type
		{
			Response = 0,
			Request = 1
		};

		enum Attributes
		{
			// Low 16 bits must match Remoting::MethodAttributes
			synchronous = 0,
			asynchronous = 1,
			unreliable = 2,
			encrypted = 4,

			// Upper 16 bits can be used for system messages
			system_message = 0xFFFF0000,
			channel_close = 0x10000,
			channel_reflect = 0x20000,
			channel_ping = 0x30000,

			// Used internally
			async_function = 0x0FFF0000
		};
	};

	class MessageHandler
	{
	protected:
		MessageHandler();
		virtual ~MessageHandler();

	public:
		int pump_requests(const OOBase::timeval_t* wait = 0, bool bOnce = false);
		bool parse_message(OOBase::CDRStream& input);
		bool call_async_function_i(const char* pszFn, void (*pfnCall)(void*,OOBase::CDRStream&), void* pParam, const OOBase::CDRStream* stream);

		struct io_result
		{
			enum type
			{
				success,
				channel_closed,
				timedout,
				failed
			};
		};

		io_result::type forward_message(Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, Omega::uint16_t flags, Omega::uint32_t seq_no, OOBase::CDRStream& message);
		io_result::type send_request(Omega::uint32_t dest_channel_id, const OOBase::CDRStream* request, OOBase::CDRStream* response, const OOBase::timeval_t* deadline, Omega::uint32_t attribs);

		void channel_closed(Omega::uint32_t channel_id, Omega::uint32_t src_channel_id);

	protected:
		io_result::type send_response(Omega::uint32_t seq_no, Omega::uint32_t dest_channel_id, Omega::uint16_t dest_thread_id, const OOBase::CDRStream& response, const OOBase::timeval_t& deadline, Omega::uint32_t attribs);

		bool start_request_threads(size_t threads);
		void shutdown_channels();
		void stop_request_threads();

		Omega::uint32_t register_channel(OOBase::RefPtr<MessageConnection>& ptrMC, Omega::uint32_t channel_id);

		virtual void process_request(OOBase::CDRStream& request, Omega::uint32_t seq_no, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs) = 0;
		virtual bool can_route(Omega::uint32_t src_channel, Omega::uint32_t dest_channel);
		virtual void on_channel_closed(Omega::uint32_t channel) = 0;
		virtual io_result::type route_off(const OOBase::CDRStream& msg, Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, Omega::uint16_t flags, Omega::uint32_t seq_no);

		void set_channel(Omega::uint32_t channel_id, Omega::uint32_t mask_id, Omega::uint32_t child_mask_id, Omega::uint32_t upstream_id);
		Omega::uint16_t classify_channel(Omega::uint32_t channel_id);

		Omega::uint32_t      m_uUpstreamChannel;

	private:
		MessageHandler(const MessageHandler&);
		MessageHandler& operator = (const MessageHandler&);

		OOBase::RWMutex      m_lock;
		Omega::uint32_t      m_uChannelId;
		Omega::uint32_t      m_uChannelMask;
		Omega::uint32_t      m_uChildMask;
		Omega::uint32_t      m_uNextChannelId;
		Omega::uint32_t      m_uNextChannelMask;
		Omega::uint32_t      m_uNextChannelShift;
		OOBase::ThreadPool   m_threadpool;

		struct ChannelHash
		{
			const MessageHandler* m_p;
			size_t hash(Omega::uint32_t v) const
			{
				return ((v & ~m_p->m_uChannelId) >> m_p->m_uNextChannelShift);
			}
		};
		friend struct ChannelHash;
		ChannelHash m_hash;

		OOBase::HashTable<Omega::uint32_t,OOBase::RefPtr<MessageConnection>,OOBase::HeapAllocator,ChannelHash> m_mapChannelIds;

		struct Message
		{
			Message() : m_payload(size_t(0)) {}
			Message(const OOBase::CDRStream& payload) : m_payload(payload) {}
			Message(size_t len) : m_payload(len) {}
			~Message() {}

			Omega::uint16_t   m_dest_thread_id;
			Omega::uint16_t   m_src_thread_id;
			Omega::uint32_t   m_src_channel_id;
			Omega::uint32_t   m_attribs;
			OOBase::timeval_t m_deadline;
			OOBase::CDRStream m_payload;
		};

		// Pooled queued members
		OOBase::Atomic<size_t>        m_waiting_threads;
		OOBase::BoundedQueue<Message> m_default_msg_queue;

		static int request_worker_fn(void* pParam);

		struct ThreadContext
		{
			Omega::uint16_t               m_thread_id;
			OOBase::BoundedQueue<Message> m_msg_queue;
			MessageHandler*               m_pHandler;

			// 'Private' thread-local data
			OOBase::timeval_t         m_deadline;
			Omega::uint32_t           m_seq_no;

			OOBase::HashTable<Omega::uint32_t,Omega::uint16_t,OOBase::LocalAllocator> m_mapChannelThreads;

			static ThreadContext* instance(MessageHandler* pHandler);

		private:
			friend class OOBase::TLSSingleton<ThreadContext,MessageHandler>;

			ThreadContext();
			~ThreadContext();

			ThreadContext(const ThreadContext&);
			ThreadContext& operator = (const ThreadContext&);
		};
		OOBase::HandleTable<Omega::uint16_t,ThreadContext*> m_mapThreadContexts;
		
		// Accessors for ThreadContext
		Omega::uint16_t insert_thread_context(ThreadContext* pContext);
		void remove_thread_context(ThreadContext* pContext);

		void send_channel_close(Omega::uint32_t dest_channel_id, Omega::uint32_t closed_channel_id);
		io_result::type queue_message(const Message& msg);
		io_result::type wait_for_response(OOBase::CDRStream& response, Omega::uint32_t seq_no, const OOBase::timeval_t* deadline, Omega::uint32_t from_channel_id);
		io_result::type send_message(Omega::uint16_t flags, Omega::uint32_t seq_no, Omega::uint32_t actual_dest_channel_id, Omega::uint32_t dest_channel_id, Message& msg);
		bool process_request_context(ThreadContext* pContext, Message& msg, Omega::uint32_t seq_no, const OOBase::timeval_t* deadline = NULL);

		void process_channel_close(Message& msg);
		void process_async_function(Message& msg);

		static void do_route_off(void* pParam, OOBase::CDRStream& input);
	};
}

#endif // OOSERVER_MSG_CONNECTION_H_INCLUDED_
