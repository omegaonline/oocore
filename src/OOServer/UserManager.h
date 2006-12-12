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
		
	void enque_request(ACE_Message_Block& mb, ACE_HANDLE handle);
	void connection_closed(SpawnedProcess::USERID key);
	
private:
	typedef ACE_Singleton<UserManager, ACE_Recursive_Thread_Mutex> USER_MANAGER;
	friend class USER_MANAGER;
	
	UserManager();
	virtual ~UserManager();
	UserManager(const UserManager&) {}
	UserManager& operator = (const UserManager&) {}

	ACE_Message_Queue<ACE_MT_SYNCH>	m_msg_queue;
	ACE_HANDLE						m_root_handle;
	
	int run_event_loop_i(u_short uPort);
	int init(u_short uPort);
	void stop_i();
	void term();

	ACE_THR_FUNC_RETURN process_requests();

	static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);
	static ACE_THR_FUNC_RETURN request_worker_fn(void*);
};

#endif // OOSERVER_USER_MANAGER_H_INCLUDED_
