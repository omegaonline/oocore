/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary and do not use precompiled headers
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_ROOT_MANAGER_H_INCLUDED_
#define OOSERVER_ROOT_MANAGER_H_INCLUDED_

#include "./LocalAcceptor.h"
#include "./RootConnection.h"
#include "./ClientConnection.h"

#include <ace/Singleton.h>
#include <ace/Condition_Thread_Mutex.h>
#include <ace/Message_Queue_T.h>

#include <map>

class RootManager : 
	public LocalAcceptor<ClientConnection>, 
	public RootBase
{
public:
	static int run_event_loop();
	static void end_event_loop();
	static void connect_client(const Session::Request& request, Session::Response& response);

	void enque_request(ACE_Message_Block& mb, ACE_HANDLE handle);
	void connection_closed(SpawnedProcess::USERID key);
		
private:
	typedef ACE_Singleton<RootManager, ACE_Recursive_Thread_Mutex> ROOT_MANAGER;
	friend class ROOT_MANAGER;

	RootManager();
	RootManager(const RootManager&) {}
	virtual ~RootManager();
	RootManager& operator = (const RootManager&) {}

	ACE_Thread_Mutex				m_lock;
	ACE_HANDLE						m_config_file;
	ACE_Message_Queue<ACE_MT_SYNCH>	m_msg_queue;

	struct UserProcess
	{
		u_short				uPort;
		SpawnedProcess*		pSpawn;
	};
	std::map<SpawnedProcess::USERID,UserProcess> m_mapSpawned;

	int run_event_loop_i();
	int init();
	void end_event_loop_i();
	void term();
	void connect_client_i(const Session::Request& request, Session::Response& response);
	void spawn_client(const Session::Request& request, Session::Response& response, UserProcess& process, SpawnedProcess::USERID key);
	ACE_THR_FUNC_RETURN process_requests();

	static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);
	static ACE_THR_FUNC_RETURN request_worker_fn(void*);
};

#endif // OOSERVER_ROOT_MANAGER_H_INCLUDED_
