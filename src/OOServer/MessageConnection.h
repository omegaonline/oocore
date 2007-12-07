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
		MessageConnection(MessageHandler* pHandler)  :
			m_pHandler(pHandler)
		{ }

		virtual ~MessageConnection()
		{ }

		ACE_CDR::UShort open(MessagePipe& pipe);

	private:
		MessageConnection() : ACE_Service_Handler() {}
		MessageConnection(const MessageConnection&) : ACE_Service_Handler() {}
		MessageConnection& operator = (const MessageConnection&) { return *this; }

        static const size_t         s_initial_read = 8;

        MessageHandler*             m_pHandler;
		MessagePipe                 m_pipe;
		size_t                      m_read_len;
		ACE_CDR::Octet              m_byte_order;
		ACE_CDR::Octet              m_version;

		bool read();

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

		bool send_request(ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* mb, ACE_InputCDR*& response, ACE_CDR::UShort timeout, ACE_CDR::UShort attribs);
		void send_response(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs);
		void pump_requests(const ACE_Time_Value* deadline = 0);

		int start(const ACE_WString& strName);
		ACE_CDR::UShort get_channel_thread_id(ACE_CDR::UShort channel);
		ACE_CDR::UShort get_pipe_channel(const MessagePipe& pipe, ACE_CDR::UShort channel);
		MessagePipe get_channel_pipe(ACE_CDR::UShort channel);
		void stop_accepting();
		void stop();
		virtual void pipe_closed(const MessagePipe& pipe);
		virtual void channel_closed(ACE_CDR::UShort channel);

		virtual void process_request(const MessagePipe& pipe, ACE_InputCDR& request, ACE_CDR::UShort src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs) = 0;

		ACE_CDR::UShort add_routing(ACE_CDR::UShort dest_channel, ACE_CDR::UShort dest_route);

		virtual int on_accept(MessagePipe& pipe, int key);

	private:
		friend class MessageConnection;
		friend class MessagePipeAsyncAcceptor<MessageHandler>;

		MessageHandler(const MessageHandler&) {}
		MessageHandler& operator = (const MessageHandler&) { return *this; }

		MessagePipeAsyncAcceptor<MessageHandler> m_connector;

		ACE_RW_Thread_Mutex  m_lock;
		ACE_CDR::UShort      m_uNextChannelId;
		struct ChannelInfo
		{
			ChannelInfo() :
				channel_id(0), lock(0)
			{}

			ChannelInfo(const MessagePipe& p, ACE_CDR::UShort c = 0, ACE_Thread_Mutex* l = 0) :
				pipe(p), channel_id(c), lock(l)
			{}

			MessagePipe         pipe;
			ACE_CDR::UShort     channel_id;
			ACE_Refcounted_Auto_Ptr<ACE_Thread_Mutex,ACE_Null_Mutex> lock;
		};
		std::map<ACE_CDR::UShort,ChannelInfo>                            m_mapChannelIds;
		std::map<MessagePipe,std::map<ACE_CDR::UShort,ACE_CDR::UShort> > m_mapReverseChannelIds;

		struct Message
		{
			MessagePipe      m_pipe;
			ACE_CDR::UShort  m_dest_channel_id;
			ACE_CDR::UShort  m_dest_thread_id;
			ACE_CDR::UShort  m_src_channel_id;
			ACE_CDR::UShort  m_src_thread_id;
			ACE_Time_Value   m_deadline;
			ACE_CDR::UShort  m_attribs;
			ACE_CDR::Boolean m_bIsRequest;
			ACE_InputCDR*    m_pPayload;
		};

		struct ThreadContext
		{
			ACE_CDR::UShort                             m_thread_id;
			ACE_Message_Queue_Ex<Message,ACE_MT_SYNCH>* m_msg_queue;
			ACE_Time_Value                              m_deadline;
			MessageHandler*                             m_pHandler;
			std::map<ACE_CDR::UShort,ACE_CDR::UShort>   m_mapChannelThreads;

			static ThreadContext* instance(Root::MessageHandler* pHandler);

		private:
			friend class ACE_TSS_Singleton<ThreadContext,ACE_Thread_Mutex>;
			friend class ACE_TSS<ThreadContext>;

			ThreadContext();
			~ThreadContext();
		};

		std::map<ACE_CDR::UShort,const ThreadContext*>  m_mapThreadContexts;
		ACE_Message_Queue_Ex<Message,ACE_MT_SYNCH>      m_default_msg_queue;

		// Accessors for ThreadContext
		ACE_CDR::UShort insert_thread_context(const ThreadContext* pContext);
		void remove_thread_context(const ThreadContext* pContext);

		// Accessors for MessageConnection
		ACE_CDR::UShort register_channel(MessagePipe& pipe);

		MessageConnection* make_handler();
		bool parse_message(Message* msg);
		bool build_header(ACE_OutputCDR& header, const Message& msg, const ACE_Message_Block* mb);
		bool wait_for_response(ACE_InputCDR*& response, const ACE_Time_Value* deadline);
	};
}

#endif // OOSERVER_MSG_CONNECTION_H_INCLUDED_
