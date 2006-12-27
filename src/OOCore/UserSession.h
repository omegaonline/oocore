#ifndef OOCORE_USER_SESSION_H_INCLUDED_
#define OOCORE_USER_SESSION_H_INCLUDED_

class UserSession
{
public:
	static int init();
	static void term();
		
	static int enqueue_request(ACE_InputCDR* input, ACE_HANDLE handle);
	static void connection_closed();
	
private:
	typedef ACE_Unmanaged_Singleton<UserSession, ACE_Recursive_Thread_Mutex> USER_SESSION;
	friend class ACE_Singleton<UserSession, ACE_Recursive_Thread_Mutex>;
	friend class USER_SESSION;
	
	UserSession();
	virtual ~UserSession();
	UserSession(const UserSession&) {}
	UserSession& operator = (const UserSession&) {}

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
    
	ACE_Thread_Mutex							m_lock;
	ACE_HANDLE									m_user_handle;
	ACE_Atomic_Op<ACE_Thread_Mutex,long>		m_next_trans_id;
	ACE_Message_Queue_Ex<Request,ACE_MT_SYNCH>	m_msg_queue;
	std::set<ACE_CDR::ULong>					m_setPendingTrans;
	int											m_pro_thrd_grp_id;

	int init_i();
	void term_i();
	void connection_closed_i();
	int get_port(u_short& uPort);
	
	int send_asynch(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* request, ACE_Time_Value* deadline);
	int send_synch(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* request, Request*& response, ACE_Time_Value* deadline);
	int send_response(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, ACE_CDR::ULong trans_id, const ACE_Message_Block* response, ACE_Time_Value* deadline);
	int pump_requests(ACE_Time_Value* deadline = 0);
		
	void process_request(Request* request, const ACE_CString& strUserId, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);
	void forward_request(Request* request, const ACE_CString& strUserId, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);
		
	static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);
	
	void process_root_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);
	void process_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);

	int wait_for_response(ACE_CDR::ULong trans_id, Request*& response, ACE_Time_Value* deadline = 0);
	int build_header(ACE_CDR::UShort dest_channel_id, ACE_CDR::ULong trans_id, ACE_OutputCDR& header, const ACE_Message_Block* mb, const ACE_Time_Value& deadline);
	bool valid_transaction(ACE_CDR::ULong trans_id);
};

#endif // OOCORE_USER_SESSION_H_INCLUDED_
