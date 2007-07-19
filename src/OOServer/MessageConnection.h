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

	class MessageConnection : public ACE_Service_Handler
	{
	public:
		MessageConnection(MessageHandler* pHandler);
		virtual ~MessageConnection();

		ACE_CDR::UShort attach(ACE_HANDLE new_handle);
		void open(ACE_HANDLE new_handle, ACE_Message_Block& mb);
		void handle_read_stream(const ACE_Asynch_Read_Stream::Result& result);

	private:
		friend class ACE_Asynch_Acceptor<MessageConnection>;

		MessageConnection() : ACE_Service_Handler() {}
		MessageConnection(const MessageConnection&) : ACE_Service_Handler() {}
		MessageConnection& operator = (const MessageConnection&) { return *this; }

        static const size_t         s_initial_read = 8;

		MessageHandler*             m_pHandler;
		ACE_Asynch_Read_Stream      m_reader;
		ACE_CDR::ULong              m_read_len;
		ACE_CDR::Octet              m_byte_order;
		ACE_CDR::Octet              m_version;

		bool read();
	};

	class MessageHandler : public ACE_Asynch_Acceptor<MessageConnection>
	{
	protected:
		MessageHandler();
		virtual ~MessageHandler() {}

		bool send_request(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb, ACE_InputCDR*& response, ACE_CDR::UShort timeout, ACE_CDR::UShort attribs);
		bool send_response(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs);
		bool pump_requests(const ACE_Time_Value* deadline = 0);

		void stop();

		virtual void process_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::UShort src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs) = 0;

		ACE_CDR::UShort add_routing(ACE_CDR::UShort dest_channel, ACE_CDR::UShort dest_route);

	private:
		friend class MessageConnection;

		MessageHandler(const MessageHandler&) : ACE_Asynch_Acceptor<MessageConnection>() {}
		MessageHandler& operator = (const MessageHandler&) { return *this; }

		ACE_RW_Thread_Mutex  m_lock;
		ACE_CDR::UShort      m_uNextChannelId;
		struct ChannelPair
		{
			ACE_HANDLE			handle;
			ACE_CDR::UShort		channel_id;
		};
		std::map<ACE_CDR::UShort,ChannelPair>                           m_mapChannelIds;
		std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> > m_mapReverseChannelIds;

		struct Message
		{
			ACE_HANDLE       m_handle;
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
			bool                                        m_bWaitingOnZero;
			ACE_Time_Value                              m_deadline;
			MessageHandler*                             m_pHandler;

			static ThreadContext* instance(Root::MessageHandler* pHandler);

		private:
			friend class ACE_TSS_Singleton<ThreadContext,ACE_Thread_Mutex>;
			friend class ACE_TSS<ThreadContext>;

			ThreadContext();
			virtual ~ThreadContext();
		};

		std::map<ACE_CDR::UShort,const ThreadContext*>  m_mapThreadContexts;

		// Accessors for ThreadContext
		ACE_CDR::UShort insert_thread_context(const ThreadContext* pContext);
		void remove_thread_context(const ThreadContext* pContext);

		// Accessors for MessageConnection
		ACE_CDR::UShort register_channel(ACE_HANDLE handle);

		MessageConnection* make_handler();
		bool parse_message(Message* msg);
		void handle_closed(ACE_HANDLE handle);
		bool build_header(ACE_OutputCDR& header, const Message& msg, const ACE_Message_Block* mb);
		bool wait_for_response(ACE_InputCDR*& response, const ACE_Time_Value* deadline);
	};
}

#endif // OOSERVER_MSG_CONNECTION_H_INCLUDED_
