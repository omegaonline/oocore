#ifndef OOSERVER_USER_MANAGER_H_INCLUDED_
#define OOSERVER_USER_MANAGER_H_INCLUDED_

#include "./LocalAcceptor.h"
#include "./RootConnection.h"
//#include "./UserConnection.h"


class UserConnection : public ACE_Service_Handler
{
};



class UserManager : 
	public LocalAcceptor<UserConnection>,
	public RootBase
{
public:
	static int run_event_loop(u_short uPort);
		
	int enque_root_request(ACE_InputCDR* input, ACE_HANDLE handle);
	void root_connection_closed(SpawnedProcess::USERID key);
	
private:
	typedef ACE_Singleton<UserManager, ACE_Recursive_Thread_Mutex> USER_MANAGER;
	friend class USER_MANAGER;
	
	UserManager();
	virtual ~UserManager();
	UserManager(const UserManager&) {}
	UserManager& operator = (const UserManager&) {}

	struct Request
	{
		bool			m_bRoot;
		ACE_HANDLE		m_handle;
		ACE_InputCDR*	m_input;
	};
	ACE_Message_Queue_Ex<Request,ACE_MT_SYNCH>	m_msg_queue;
		
	int run_event_loop_i(u_short uPort);
	int init(u_short uPort);
	void stop_i();
	void term();
	
	int wait_for_response(ACE_CDR::ULong trans_id, ACE_HANDLE handle, ACE_InputCDR*& response, ACE_Time_Value* deadline = 0);
	void process_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);
	void process_root_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);
	int send_synch(ACE_HANDLE handle, const ACE_OutputCDR& request, ACE_InputCDR*& response, ACE_Time_Value* wait);
	int send_asynch(ACE_HANDLE handle, const ACE_OutputCDR& request, ACE_Time_Value* wait);
	ACE_CDR::ULong build_header(ACE_OutputCDR& header, const ACE_OutputCDR& request, const ACE_Time_Value& deadline);
	ACE_CDR::ULong next_trans_id();
	bool expired_request(ACE_CDR::ULong trans_id);
	void cancel_trans_id(ACE_CDR::ULong trans_id);

	static const unsigned int header_padding = 16;

	ACE_THR_FUNC_RETURN process_requests();

	static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);
	static ACE_THR_FUNC_RETURN request_worker_fn(void*);
};

#endif // OOSERVER_USER_MANAGER_H_INCLUDED_

