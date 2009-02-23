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

#include "./MessagePipe.h"

namespace Root
{
	class MessageHandler;

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	class MessageConnection : public ACE_Service_Handler
#else
	class MessageConnection : public ACE_Event_Handler
#endif
	{
	public:
		MessageConnection(MessageHandler* pHandler);
		virtual ~MessageConnection();

		ACE_CDR::ULong open(const ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex>& pipe, ACE_CDR::ULong channel_id = 0, bool bStart = true);
		bool read(ACE_Message_Block* mb = 0);

	private:
#if defined(ACE_HAS_WIN32_NAMED_PIPES)
		MessageConnection() : ACE_Service_Handler() {}
		MessageConnection(const MessageConnection&) : ACE_Service_Handler() {}
#else
		MessageConnection() : ACE_Event_Handler() {}
		MessageConnection(const MessageConnection&) : ACE_Event_Handler() {}
#endif
		MessageConnection& operator = (const MessageConnection&) { return *this; }

		MessageHandler*                                       m_pHandler;
		ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex> m_pipe;
		size_t                                                m_chunk_size;
		ACE_CDR::ULong                                        m_channel_id;

		bool process_read(ACE_Message_Block* mb);

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
		ACE_Asynch_Read_File m_reader;

		void handle_read_file(const ACE_Asynch_Read_File::Result& result);
#else
		ACE_Thread_Mutex   m_lock;
		ACE_Message_Block* m_mb;

		int handle_input(ACE_HANDLE);
		int handle_close(ACE_HANDLE, ACE_Reactor_Mask);
#endif
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
		int pump_requests(const ACE_Time_Value* deadline = 0, bool bOnce = false);

		void channel_closed(ACE_CDR::ULong channel_id, ACE_CDR::ULong src_channel_id);
		bool forward_message(ACE_CDR::ULong src_channel_id, ACE_CDR::ULong dest_channel_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs, ACE_CDR::UShort dest_thread_id, ACE_CDR::UShort src_thread_id, ACE_CDR::UShort flags, ACE_CDR::ULong seq_no, const ACE_Message_Block* mb);
		bool send_request(ACE_CDR::ULong dest_channel_id, const ACE_Message_Block* mb, ACE_InputCDR*& response, ACE_Time_Value* deadline, ACE_CDR::ULong attribs);
				
	protected:
		bool send_response(ACE_CDR::ULong seq_no, ACE_CDR::ULong dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs);
		bool call_async_function_i(void (*pfnCall)(void*,ACE_InputCDR&), void* pParam, const ACE_Message_Block* mb);
		
		int start();
		void close();
		void stop();

		virtual void process_request(ACE_InputCDR& request, ACE_CDR::ULong seq_no, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs) = 0;
		virtual bool can_route(ACE_CDR::ULong src_channel, ACE_CDR::ULong dest_channel);
		virtual bool on_channel_open(ACE_CDR::ULong channel);
		virtual void on_channel_closed(ACE_CDR::ULong channel) = 0;
		virtual bool route_off(ACE_InputCDR& msg, ACE_CDR::ULong src_channel_id, ACE_CDR::ULong dest_channel_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs, ACE_CDR::UShort dest_thread_id, ACE_CDR::UShort src_thread_id, ACE_CDR::UShort flags, ACE_CDR::ULong seq_no);

		void set_channel(ACE_CDR::ULong channel_id, ACE_CDR::ULong mask_id, ACE_CDR::ULong child_mask_id, ACE_CDR::ULong upstream_id);
		ACE_CDR::UShort classify_channel(ACE_CDR::ULong channel_id);
		
	private:
		friend class MessageConnection;
		friend class MessagePipeAsyncAcceptor<MessageHandler>;

		MessageHandler(const MessageHandler&) {}
		MessageHandler& operator = (const MessageHandler&) { return *this; }

		ACE_RW_Thread_Mutex  m_lock;
		int                  m_req_thrd_grp_id;
		int                  m_pro_thrd_grp_id;
		ACE_CDR::ULong       m_uChannelId;
		ACE_CDR::ULong       m_uChannelMask;
		ACE_CDR::ULong       m_uChildMask;
		ACE_CDR::ULong       m_uUpstreamChannel;
		ACE_CDR::ULong       m_uNextChannelId;
		ACE_CDR::ULong       m_uNextChannelMask;
		ACE_CDR::ULong       m_uNextChannelShift;

		std::map<ACE_CDR::ULong,ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex> > m_mapChannelIds;

		struct Message
		{
			ACE_CDR::UShort                                        m_dest_thread_id;
			ACE_CDR::UShort                                        m_src_thread_id;
			ACE_CDR::ULong                                         m_src_channel_id;
			ACE_CDR::ULong                                         m_attribs;
			ACE_Time_Value                                         m_deadline;
			ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Thread_Mutex> m_ptrPayload;
		};

		struct ThreadContext
		{
			bool                                        m_bPrivate;
			ACE_CDR::UShort                             m_thread_id;
			ACE_Message_Queue_Ex<Message,ACE_MT_SYNCH>* m_msg_queue;
			MessageHandler*                             m_pHandler;

			// Transient data
			size_t                                      m_usage_count;
			std::map<ACE_CDR::ULong,ACE_CDR::UShort>    m_mapChannelThreads;
			ACE_Time_Value                              m_deadline;
			ACE_CDR::ULong                              m_seq_no;

			static ThreadContext* instance(Root::MessageHandler* pHandler);

		private:
			friend class ACE_TSS_Singleton<ThreadContext,ACE_Recursive_Thread_Mutex>;
			friend class ACE_TSS<ThreadContext>;

			ThreadContext();
			~ThreadContext();
		};

		std::map<ACE_CDR::UShort,const ThreadContext*>  m_mapThreadContexts;

		struct WorkerInfo
		{
			ACE_Event  m_Event;
			Message*   m_msg;
		};
		ACE_Message_Queue_Ex<WorkerInfo,ACE_MT_SYNCH>  m_thread_queue;

		// Accessors for ThreadContext
		ACE_CDR::UShort insert_thread_context(const ThreadContext* pContext);
		void remove_thread_context(const ThreadContext* pContext);

		// Accessors for MessageConnection
		ACE_CDR::ULong register_channel(const ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex>& pipe, ACE_CDR::ULong channel_id);

		bool send_channel_close(ACE_CDR::ULong dest_channel_id, ACE_CDR::ULong closed_channel_id);
		bool route_message(MessageHandler::Message* msg);
		bool parse_message(ACE_InputCDR& input, const ACE_Message_Block* mb);
		bool build_header(ACE_OutputCDR& header, ACE_CDR::UShort flags, ACE_CDR::ULong seq_no, ACE_CDR::ULong dest_channel_id, const Message& msg, const ACE_Message_Block* mb);
		bool wait_for_response(ACE_InputCDR*& response, ACE_CDR::ULong seq_no, const ACE_Time_Value* deadline, ACE_CDR::ULong from_channel_id);
		bool send_message(ACE_CDR::UShort flags, ACE_CDR::ULong seq_no, ACE_CDR::ULong actual_dest_channel_id, ACE_CDR::ULong dest_channel_id, const Message& msg, const ACE_Message_Block* mb);

		void process_channel_close(Message* msg);
		void process_async_function(Message* msg);

		static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);
		static ACE_THR_FUNC_RETURN request_worker_fn(void* pParam);

		static void do_route_off(void* pParam, ACE_InputCDR& input);
	};
}

#endif // OOSERVER_MSG_CONNECTION_H_INCLUDED_
