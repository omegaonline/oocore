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
#include "./RequestHandler.h"
#include "./RootConnection.h"
#include "./ClientConnection.h"

#include <ace/Singleton.h>

#include <map>

class SpawnedProcess;

class RootManager : 
	public LocalAcceptor<ClientConnection>, 
	public RootBase,
	public RequestHandler<RequestBase>
{
public:
	static int run_event_loop();
	static void end_event_loop();
	static void connect_client(const Session::Request& request, Session::Response& response);
		
private:
	typedef ACE_Singleton<RootManager, ACE_Recursive_Thread_Mutex> ROOT_MANAGER;
	friend class ROOT_MANAGER;

	RootManager();
	RootManager(const RootManager&) {}
	virtual ~RootManager();
	RootManager& operator = (const RootManager&) {}

	ACE_Thread_Mutex				m_lock;
	ACE_HANDLE						m_config_file;

	struct UserProcess
	{
		u_short			uPort;
		SpawnedProcess*	pSpawn;
	};	
	std::map<ACE_CString,UserProcess>		m_mapUserProcesses;
	std::map<ACE_HANDLE,ACE_CString>		m_mapUserIds;
	ACE_CDR::UShort							m_uNextChannelId;
	struct ChannelPair
	{
		ACE_HANDLE			handle;
		ACE_CDR::UShort		channel;
	};
	std::map<ACE_CDR::UShort,ChannelPair>		m_mapChannelIds;
	std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >	m_mapReverseChannelIds;
	
	int run_event_loop_i();
	int init();
	void end_event_loop_i();
	void term();
	void connect_client_i(const Session::Request& request, Session::Response& response);
	void spawn_client(const Session::Request& request, Session::Response& response, const ACE_CString& key);
	
	int enqueue_root_request(ACE_InputCDR* input, ACE_HANDLE handle);
	void root_connection_closed(const ACE_CString& key, ACE_HANDLE handle);
	void process_request(RequestBase* request, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);
	void process_root_request(RequestBase* request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);
	void forward_request(RequestBase* request, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);

	static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);
	static ACE_THR_FUNC_RETURN request_worker_fn(void*);
};

#endif // OOSERVER_ROOT_MANAGER_H_INCLUDED_
