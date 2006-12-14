#ifndef OOSERVER_USER_MANAGER_H_INCLUDED_
#define OOSERVER_USER_MANAGER_H_INCLUDED_

#include "./LocalAcceptor.h"
#include "./RequestHandler.h"
#include "./RootConnection.h"

//#include "./UserConnection.h"

class UserConnection : public ACE_Service_Handler
{
};



class UserRequest : public RequestBase
{
public:
	UserRequest(ACE_HANDLE handle, ACE_InputCDR* input) : 
	  RequestBase(handle,input)
	{}

	bool	m_bRoot;
};

class UserManager : 
	public LocalAcceptor<UserConnection>,
	public RootBase,
	public RequestHandler<UserRequest>
{
public:
	static int run_event_loop(u_short uPort);
	
private:
	typedef ACE_Singleton<UserManager, ACE_Recursive_Thread_Mutex> USER_MANAGER;
	friend class USER_MANAGER;
	
	UserManager();
	virtual ~UserManager();
	UserManager(const UserManager&) {}
	UserManager& operator = (const UserManager&) {}
	
	int run_event_loop_i(u_short uPort);
	int init(u_short uPort);
	void stop_i();
	void term();

	int enque_root_request(ACE_InputCDR* input, ACE_HANDLE handle);
	void root_connection_closed(const SpawnedProcess::USERID& key, ACE_HANDLE handle);
	void process_request(UserRequest* request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);
	
	static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);
	static ACE_THR_FUNC_RETURN request_worker_fn(void*);

	void process_root_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);
	void process_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);
};

#endif // OOSERVER_USER_MANAGER_H_INCLUDED_


