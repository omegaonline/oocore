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

		bool enqueue_request(ACE_InputCDR* input, ACE_HANDLE handle);
		void connection_closed();

	private:
		friend class Channel;
		friend class ACE_Singleton<UserSession, ACE_Recursive_Thread_Mutex>;
		friend class ACE_Unmanaged_Singleton<UserSession, ACE_Recursive_Thread_Mutex>;
		typedef ACE_Unmanaged_Singleton<UserSession, ACE_Recursive_Thread_Mutex> USER_SESSION;

		UserSession();
		virtual ~UserSession();
		UserSession(const UserSession&) {}
		UserSession& operator = (const UserSession&) { return *this; }

		class Request
		{
		public:
			Request(ACE_HANDLE handle, ACE_InputCDR* input) :
				m_handle(handle),
				m_input(input)
			{}

			virtual ~Request()
			{
				if (m_input)
					delete m_input;
			}

			ACE_HANDLE handle() const
			{
				return m_handle;
			}

			ACE_InputCDR* input() const
			{
				return m_input;
			}

		private:
			ACE_HANDLE		m_handle;
			ACE_InputCDR*	m_input;
		};

		ACE_Recursive_Thread_Mutex                                                 m_lock;
		int                                                                        m_pro_thrd_grp_id;
		ACE_HANDLE                                                                 m_user_handle;
		ACE_Atomic_Op<ACE_Thread_Mutex,long>                                       m_next_trans_id;
		ACE_Message_Queue_Ex<Request,ACE_MT_SYNCH>                                 m_msg_queue;
		std::set<ACE_CDR::ULong>                                                   m_setPendingTrans;
		std::map<ACE_CDR::UShort,OTL::ObjectPtr<Omega::Remoting::IObjectManager> > m_mapOMs;

		// Accessors for Channel
		bool send_asynch(ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* request, ACE_Time_Value* deadline);
		bool send_synch(ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* request, Request*& response, ACE_Time_Value* deadline);
		bool send_response(ACE_CDR::UShort dest_channel_id, ACE_CDR::ULong trans_id, const ACE_Message_Block* response, ACE_Time_Value* deadline);

		// Proper private members
		bool init_i(Omega::string_t& strSource);
		Omega::IException* bootstrap();
		ACE_CString get_bootstrap_filename();
		void term_i();
		bool get_port(u_short& uPort, Omega::string_t& strSource);
		bool launch_server(Omega::string_t& strSource);
		bool pump_requests(ACE_Time_Value* deadline = 0);
		void process_request(Request* request, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);
		bool wait_for_response(ACE_CDR::ULong trans_id, Request*& response, ACE_Time_Value* deadline = 0);
		bool build_header(ACE_CDR::UShort dest_channel_id, ACE_CDR::ULong trans_id, ACE_OutputCDR& header, const ACE_Message_Block* mb, const ACE_Time_Value& deadline);
		bool valid_transaction(ACE_CDR::ULong trans_id);
		OTL::ObjectPtr<Omega::Remoting::IObjectManager> get_object_manager(ACE_CDR::UShort src_channel_id);

		static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);
	};
}

#endif // OOCORE_USER_SESSION_H_INCLUDED_
