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
//	***** THIS IS A SECURE MODULE *****
//
//	It can be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_MSG_CONNECTION_H_INCLUDED_
#define OOSERVER_MSG_CONNECTION_H_INCLUDED_

namespace Root
{
	class MessageHandler;

	class MessageConnection : public OOSvrBase::IOHandler
	{
	public:
		MessageConnection(MessageHandler* pHandler);
		virtual ~MessageConnection();

		void attach(OOSvrBase::AsyncSocket* pSocket);
		void set_channel_id(Omega::uint32_t channel_id);

		void close();
		bool read();
		bool send(OOBase::Buffer* pBuffer);

	private:
		MessageConnection(const MessageConnection&) {}
		MessageConnection& operator = (const MessageConnection&) { return *this; }

		MessageHandler*         m_pHandler;
		OOSvrBase::AsyncSocket* m_pSocket;
		Omega::uint32_t         m_channel_id;

		virtual void on_read(OOSvrBase::AsyncSocket* pSocket, OOBase::Buffer* buffer, int err);
		virtual void on_write(OOSvrBase::AsyncSocket* pSocket, OOBase::Buffer* buffer, int err);
		virtual void on_closed(OOSvrBase::AsyncSocket* pSocket);
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

			// Used internally
			async_function = 0x0FFF0000
		};
	};

	class MessageHandler
	{
	protected:
		MessageHandler();
		virtual ~MessageHandler() {}

	public:
		int pump_requests(const OOBase::timeval_t* wait = 0, bool bOnce = false);
		bool parse_message(OOBase::CDRStream& input, size_t mark_rd);
		bool call_async_function_i(void (*pfnCall)(void*,OOBase::CDRStream&), void* pParam, const OOBase::CDRStream* stream);

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

		io_result::type forward_message(Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, Omega::uint16_t flags, Omega::uint32_t seq_no, const OOBase::CDRStream& message);
		io_result::type send_request(Omega::uint32_t dest_channel_id, const OOBase::CDRStream* request, OOBase::SmartPtr<OOBase::CDRStream>& response, OOBase::timeval_t* deadline, Omega::uint32_t attribs);

		void channel_closed(Omega::uint32_t channel_id, Omega::uint32_t src_channel_id);
				
	protected:
		io_result::type send_response(Omega::uint32_t seq_no, Omega::uint32_t dest_channel_id, Omega::uint16_t dest_thread_id, const OOBase::CDRStream& response, const OOBase::timeval_t& deadline, Omega::uint32_t attribs);
				
		bool start();
		void close();
		void stop();

		Omega::uint32_t register_channel(OOBase::SmartPtr<MessageConnection>& ptrMC, Omega::uint32_t channel_id);
		
		virtual void process_request(OOBase::CDRStream& request, Omega::uint32_t seq_no, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs) = 0;
		virtual bool can_route(Omega::uint32_t src_channel, Omega::uint32_t dest_channel);
		virtual bool on_channel_open(Omega::uint32_t channel);
		virtual void on_channel_closed(Omega::uint32_t channel) = 0;
		virtual io_result::type route_off(const OOBase::CDRStream& msg, Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, Omega::uint16_t flags, Omega::uint32_t seq_no);

		void set_channel(Omega::uint32_t channel_id, Omega::uint32_t mask_id, Omega::uint32_t child_mask_id, Omega::uint32_t upstream_id);
		Omega::uint16_t classify_channel(Omega::uint32_t channel_id);
		
	private:
		MessageHandler(const MessageHandler&) {}
		MessageHandler& operator = (const MessageHandler&) { return *this; }

		OOBase::RWMutex      m_lock;
		Omega::uint32_t      m_uChannelId;
		Omega::uint32_t      m_uChannelMask;
		Omega::uint32_t      m_uChildMask;
		Omega::uint32_t      m_uUpstreamChannel;
		Omega::uint32_t      m_uNextChannelId;
		Omega::uint32_t      m_uNextChannelMask;
		Omega::uint32_t      m_uNextChannelShift;

		std::map<Omega::uint32_t,OOBase::SmartPtr<MessageConnection> > m_mapChannelIds;

		struct Message
		{
			Omega::uint16_t   m_dest_thread_id;
			Omega::uint16_t   m_src_thread_id;
			Omega::uint32_t   m_src_channel_id;
			Omega::uint32_t   m_attribs;
			OOBase::timeval_t m_deadline;
			OOBase::CDRStream m_payload;
		};

		// Pooled queued members
		OOBase::AtomicInt<Omega::uint32_t>               m_usage_count;
		std::list<OOBase::Thread*>                       m_threads;
		OOBase::BoundedQueue<OOBase::SmartPtr<Message> > m_default_msg_queue;

		bool start_thread();
		static int request_worker_fn(void* pParam);

		struct ThreadContext
		{
			Omega::uint16_t                                  m_thread_id;
			OOBase::BoundedQueue<OOBase::SmartPtr<Message> > m_msg_queue;
			MessageHandler*                                  m_pHandler;

			// Transient data
			size_t                                    m_usage_count;
			std::map<Omega::uint32_t,Omega::uint16_t> m_mapChannelThreads;
			OOBase::timeval_t                         m_deadline;
			Omega::uint32_t                           m_seq_no;

			static ThreadContext* instance(MessageHandler* pHandler);

		private:
			friend class OOBase::TLSSingleton<ThreadContext>;
			
			ThreadContext();
			~ThreadContext();

			ThreadContext(const ThreadContext&) {}
			ThreadContext& operator = (const ThreadContext&) { return *this; }
		};
		friend struct ThreadContext;
		std::map<Omega::uint16_t,ThreadContext*> m_mapThreadContexts;

		// Accessors for ThreadContext
		Omega::uint16_t insert_thread_context(ThreadContext* pContext);
		void remove_thread_context(ThreadContext* pContext);

		void send_channel_close(Omega::uint32_t dest_channel_id, Omega::uint32_t closed_channel_id);
		io_result::type queue_message(OOBase::SmartPtr<Message>& msg);
		io_result::type wait_for_response(OOBase::SmartPtr<OOBase::CDRStream>& response, Omega::uint32_t seq_no, const OOBase::timeval_t* deadline, Omega::uint32_t from_channel_id);
		io_result::type send_message(Omega::uint16_t flags, Omega::uint32_t seq_no, Omega::uint32_t actual_dest_channel_id, Omega::uint32_t dest_channel_id, const Message& msg);

		void process_channel_close(const OOBase::SmartPtr<Message>& msg);
		void process_async_function(const OOBase::SmartPtr<Message>& msg);

		static void do_route_off(void* pParam, OOBase::CDRStream& input);
	};
}

#endif // OOSERVER_MSG_CONNECTION_H_INCLUDED_
