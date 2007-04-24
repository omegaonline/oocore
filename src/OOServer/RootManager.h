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

namespace Root
{

class SpawnedProcess;

class Manager :
	public LocalAcceptor<ClientConnection>,
	public RootBase,
	public RequestHandler<RequestBase>
{
public:
	static int run();
	static void end();
	static int connect_client(uid_t uid, u_short& uNewPort);
	static ACE_Configuration_Heap& get_registry();

private:
	friend class ACE_Singleton<Manager, ACE_Recursive_Thread_Mutex>;
	typedef ACE_Singleton<Manager, ACE_Recursive_Thread_Mutex> ROOT_MANAGER;

	Manager();
	Manager(const Manager&) {}
	virtual ~Manager();
	Manager& operator = (const Manager&) { return *this; }

	ACE_Thread_Mutex				m_lock;
	ACE_HANDLE						m_config_file;

	struct UserProcess
	{
		u_short			uPort;
		SpawnedProcess*	pSpawn;
	};
	std::map<ACE_CString,UserProcess>  m_mapUserProcesses;
	std::map<ACE_HANDLE,ACE_CString>   m_mapUserIds;
	ACE_CDR::UShort                    m_uNextChannelId;
	struct ChannelPair
	{
		ACE_HANDLE			handle;
		ACE_CDR::UShort		channel;
	};
	std::map<ACE_CDR::UShort,ChannelPair>                           m_mapChannelIds;
	std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> > m_mapReverseChannelIds;

	int run_event_loop_i();
	int init();
	int init_registry();
	ACE_TString get_bootstrap_filename();
	int bootstrap_client(ACE_SOCK_STREAM& stream, bool bSandbox);
	void end_event_loop_i();
	void term();
	int connect_client_i(uid_t uid, u_short& uNewPort);
	int spawn_sandbox();
	int spawn_client(uid_t uid, const ACE_CString& key);

	int enqueue_root_request(ACE_InputCDR* input, ACE_HANDLE handle);
	void root_connection_closed(const ACE_CString& key, ACE_HANDLE handle);
	void process_request(RequestBase* request, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);
	void process_root_request(RequestBase* request, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);
	void forward_request(RequestBase* request, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline);
	int access_check(ACE_HANDLE handle, const char* pszObject, ACE_UINT32 mode, bool& bAllowed);

	ACE_Configuration_Heap         m_registry;
	ACE_CString                    m_strRegistry;
	ACE_Thread_Mutex               m_registry_lock;

	bool registry_open_section(RequestBase* request, ACE_Configuration_Section_Key& key, bool bAccessCheck = false);
	bool registry_open_value(RequestBase* request, ACE_Configuration_Section_Key& key, ACE_CString& strValue, bool bAccessCheck = false);
	void registry_key_exists(RequestBase* request, ACE_OutputCDR& response);
	void registry_create_key(RequestBase* request, ACE_OutputCDR& response);
	void registry_delete_key(RequestBase* request, ACE_OutputCDR& response);
	void registry_enum_subkeys(RequestBase* request, ACE_OutputCDR& response);
	void registry_value_type(RequestBase* request, ACE_OutputCDR& response);
	void registry_get_string_value(RequestBase* request, ACE_OutputCDR& response);
	void registry_get_uint_value(RequestBase* request, ACE_OutputCDR& response);
	void registry_set_string_value(RequestBase* request, ACE_OutputCDR& response);
	void registry_set_uint_value(RequestBase* request, ACE_OutputCDR& response);
	void registry_enum_values(RequestBase* request, ACE_OutputCDR& response);
	void registry_delete_value(RequestBase* request, ACE_OutputCDR& response);

	static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);
	static ACE_THR_FUNC_RETURN request_worker_fn(void*);
};

}

#endif // OOSERVER_ROOT_MANAGER_H_INCLUDED_
