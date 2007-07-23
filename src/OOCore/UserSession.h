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
		int                 m_thrd_grp_id;
		ACE_SOCK_Stream     m_stream;

		struct OMInfo
		{
			OTL::ObjectPtr<Omega::Remoting::IObjectManager> m_ptrOM;
			OTL::ObjectPtr<OTL::ObjectImpl<Channel> >       m_ptrChannel;
		};
		std::map<ACE_CDR::UShort,OMInfo> m_mapOMs;

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
			
			static ThreadContext* instance();

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

		// Accessors for Channel
		bool send_request(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* request, ACE_InputCDR*& response, ACE_CDR::UShort timeout, ACE_CDR::UShort attribs);
		
		// Proper private members
		bool init_i(Omega::string_t& strSource);
		void term_i();
		Omega::IException* bootstrap();
		ACE_WString get_bootstrap_filename();
		bool discover_server_port(u_short& uPort, Omega::string_t& strSource);
		bool launch_server(Omega::string_t& strSource);

		int run_read_loop();
		bool pump_requests(const ACE_Time_Value* deadline = 0);
		void process_request(OMInfo& oim, const UserSession::Message* pMsg, const ACE_Time_Value& deadline);
		bool wait_for_response(ACE_InputCDR*& response, const ACE_Time_Value* deadline);
		bool build_header(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, ACE_OutputCDR& header, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, bool bIsRequest, ACE_CDR::UShort attribs);
		bool send_response(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* response);
		OMInfo get_object_manager(ACE_CDR::UShort src_channel_id);

		static ACE_THR_FUNC_RETURN io_worker_fn(void* pParam);
	};
}

#endif // OOCORE_USER_SESSION_H_INCLUDED_
