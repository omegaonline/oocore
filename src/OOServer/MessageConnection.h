///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
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

	class MessageConnection : public ACE_Service_Handler
	{
	public:
		MessageConnection(MessageHandler* pHandler);
		virtual ~MessageConnection();

		ACE_CDR::ULong open(const ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Null_Mutex>& pipe, ACE_CDR::ULong channel_id = 0, bool bStart = true);
		bool read();

	private:
		MessageConnection() : ACE_Service_Handler() {}
		MessageConnection(const MessageConnection&) : ACE_Service_Handler() {}
		MessageConnection& operator = (const MessageConnection&) { return *this; }

        static const size_t         s_initial_read = ACE_CDR::LONG_SIZE;

        MessageHandler*                                     m_pHandler;
		ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Null_Mutex> m_pipe;
		size_t                                              m_read_len;
		ACE_CDR::ULong                                      m_channel_id;

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
		ACE_Asynch_Read_File        m_reader;
		void handle_read_file(const ACE_Asynch_Read_File::Result& result);
#else
		ACE_Asynch_Read_Stream      m_reader;
		void handle_read_stream(const ACE_Asynch_Read_Stream::Result& result);
#endif
	};

	class MessageHandler
	{
	protected:
		MessageHandler();
		virtual ~MessageHandler() {}

		bool send_request(ACE_CDR::ULong dest_channel_id, const ACE_Message_Block* mb, ACE_InputCDR*& response, ACE_CDR::UShort timeout, ACE_CDR::ULong attribs);
		void send_response(ACE_CDR::ULong dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs);
		void pump_requests(const ACE_Time_Value* deadline = 0);

		void close();
		void stop();
		
		virtual bool channel_open(ACE_CDR::ULong channel);
		virtual void channel_closed(ACE_CDR::ULong channel) = 0;

		void set_channel(ACE_CDR::ULong channel_id, ACE_CDR::ULong mask_id, ACE_CDR::ULong child_mask_id, ACE_CDR::ULong upstream_id);
		ACE_CDR::UShort classify_channel(ACE_CDR::ULong channel_id);

		virtual void process_request(ACE_InputCDR& request, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs) = 0;
		
	private:
		friend class MessageConnection;
		friend class MessagePipeAsyncAcceptor<MessageHandler>;

		MessageHandler(const MessageHandler&) {}
		MessageHandler& operator = (const MessageHandler&) { return *this; }

		ACE_RW_Thread_Mutex  m_lock;
		ACE_CDR::ULong       m_uChannelId;
		ACE_CDR::ULong       m_uChannelMask;
		ACE_CDR::ULong       m_uChildMask;
		ACE_CDR::ULong       m_uUpstreamChannel;
		ACE_CDR::ULong       m_uNextChannelId;
		ACE_CDR::ULong       m_uNextChannelMask;
		ACE_CDR::ULong       m_uNextChannelShift;
		
		struct ChannelInfo
		{
			ChannelInfo() :
				lock(0)
			{}

			ChannelInfo(const ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Null_Mutex>& p, ACE_Thread_Mutex* l = 0) :
				pipe(p), lock(l)
			{}

			ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Null_Mutex>      pipe;
			ACE_Refcounted_Auto_Ptr<ACE_Thread_Mutex,ACE_Null_Mutex> lock;
		};
		std::map<ACE_CDR::ULong,ChannelInfo> m_mapChannelIds;
		
		struct Message
		{
			enum Flags
			{
				Response = 0,
				Request = 1
			};

			ACE_CDR::UShort  m_dest_thread_id;
			ACE_CDR::ULong   m_src_channel_id;
			ACE_CDR::UShort  m_src_thread_id;
			ACE_Time_Value   m_deadline;
			ACE_CDR::ULong   m_attribs;
			ACE_InputCDR*    m_pPayload;
		};

		struct ThreadContext
		{
			ACE_CDR::UShort                             m_thread_id;
			ACE_Message_Queue_Ex<Message,ACE_MT_SYNCH>* m_msg_queue;
			MessageHandler*                             m_pHandler;

			// Transient data
			std::map<ACE_CDR::ULong,ACE_CDR::UShort>    m_mapChannelThreads;
			ACE_Time_Value                              m_deadline;

			static ThreadContext* instance(Root::MessageHandler* pHandler);

		private:
			friend class ACE_TSS_Singleton<ThreadContext,ACE_Thread_Mutex>;
			friend class ACE_TSS<ThreadContext>;

			ThreadContext();
			~ThreadContext();
		};

		ACE_Atomic_Op<ACE_Thread_Mutex,long>            m_consumers;
		std::map<ACE_CDR::UShort,const ThreadContext*>  m_mapThreadContexts;
		ACE_Message_Queue_Ex<Message,ACE_MT_SYNCH>      m_default_msg_queue;

		// Accessors for ThreadContext
		ACE_CDR::UShort insert_thread_context(const ThreadContext* pContext);
		void remove_thread_context(const ThreadContext* pContext);

		// Accessors for MessageConnection
		ACE_CDR::ULong register_channel(const ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Null_Mutex>& pipe, ACE_CDR::ULong channel_id);
		void pipe_closed(ACE_CDR::ULong channel_id);

		bool parse_message(const ACE_Message_Block* mb);
		bool build_header(ACE_OutputCDR& header, ACE_CDR::UShort flags, ACE_CDR::ULong dest_channel_id, const Message& msg, const ACE_Message_Block* mb);
		bool wait_for_response(ACE_InputCDR*& response, const ACE_Time_Value* deadline);
	};
}

#endif // OOSERVER_MSG_CONNECTION_H_INCLUDED_
