#ifndef OOCORE_USER_SESSION_H_INCLUDED_
#define OOCORE_USER_SESSION_H_INCLUDED_

#include "./Channel.h"

namespace OOCore
{
	class UserSession
	{
	public:
		static Omega::IException* init();
		static void term();

	private:
		friend class Channel;
		friend class ThreadContext;
		friend class ACE_Singleton<UserSession, ACE_Thread_Mutex>;
		friend class ACE_Unmanaged_Singleton<UserSession, ACE_Thread_Mutex>;
		typedef ACE_Unmanaged_Singleton<UserSession, ACE_Thread_Mutex> USER_SESSION;

		UserSession();
		virtual ~UserSession();
		UserSession(const UserSession&) {}
		UserSession& operator = (const UserSession&) { return *this; }

		ACE_RW_Thread_Mutex m_lock;
		ACE_Thread_Mutex    m_send_lock;
		int                 m_thrd_grp_id;
		ACE_SOCK_Stream     m_stream;

		std::map<ACE_CDR::UShort,OTL::ObjectPtr<Omega::Remoting::IObjectManager> > m_mapOMs;

		struct Message
		{
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
			std::map<ACE_CDR::UShort,ACE_CDR::UShort>   m_mapChannelThreads;
			
			static ThreadContext* instance();

		private:
			friend class ACE_TSS_Singleton<ThreadContext,ACE_Thread_Mutex>;
			friend class ACE_TSS<ThreadContext>;

			ThreadContext();
			virtual ~ThreadContext();
		};
		
		std::map<ACE_CDR::UShort,const ThreadContext*>  m_mapThreadContexts;
		ACE_Message_Queue_Ex<Message,ACE_MT_SYNCH>      m_default_msg_queue;

		// Accessors for ThreadContext
		ACE_CDR::UShort insert_thread_context(const ThreadContext* pContext);
		void remove_thread_context(const ThreadContext* pContext);

		// Accessors for Channel
		bool send_request(ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* request, ACE_InputCDR*& response, ACE_CDR::UShort timeout, ACE_CDR::UShort attribs);
		
		// Proper private members
		bool init_i(Omega::string_t& strSource);
		void term_i();
		Omega::IException* bootstrap();
		ACE_WString get_bootstrap_filename();
		bool discover_server_port(u_short& uPort, Omega::string_t& strSource);
		bool launch_server(Omega::string_t& strSource);

		int run_read_loop();
		void pump_requests(const ACE_Time_Value* deadline = 0);
		void process_request(OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOM, const UserSession::Message* pMsg, const ACE_Time_Value& deadline);
		bool wait_for_response(ACE_InputCDR*& response, const ACE_Time_Value* deadline);
		bool build_header(const ThreadContext* pContext, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, ACE_OutputCDR& header, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, bool bIsRequest, ACE_CDR::UShort attribs);
		void send_response(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* response);
		OTL::ObjectPtr<Omega::Remoting::IObjectManager> get_object_manager(ACE_CDR::UShort src_channel_id);

		static ACE_THR_FUNC_RETURN io_worker_fn(void* pParam);
	};
}

#endif // OOCORE_USER_SESSION_H_INCLUDED_
