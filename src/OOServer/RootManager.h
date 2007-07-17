/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_ROOT_MANAGER_H_INCLUDED_
#define OOSERVER_ROOT_MANAGER_H_INCLUDED_

#include "./MessageConnection.h"
#include "./ClientConnection.h"

namespace Root
{
	class SpawnedProcess;

	class Manager : 
		public MessageHandler,
		public ACE_Asynch_Acceptor<ClientConnection>
	{
	public:
		static int run(int argc, ACE_TCHAR* argv[]);
		static void end();
		static bool connect_client(uid_t uid, u_short& uNewPort, ACE_CString& strSource);
		static ACE_Configuration_Heap& get_registry();
		static bool install(int argc, ACE_TCHAR* argv[]);
		static bool uninstall();

	private:
		friend class ACE_Singleton<Manager,ACE_Thread_Mutex>;
		typedef ACE_Singleton<Manager, ACE_Thread_Mutex> ROOT_MANAGER;

		Manager();
		Manager(const Manager&) {}
		virtual ~Manager();
		Manager& operator = (const Manager&) { return *this; }

		ACE_RW_Thread_Mutex  m_lock;
		ACE_HANDLE           m_config_file;
		//bool                 m_bThreaded;

		struct UserProcess
		{
			u_short			uPort;
			SpawnedProcess*	pSpawn;
		};
		std::map<ACE_CString,UserProcess>  m_mapUserProcesses;
		std::map<ACE_HANDLE,ACE_CString>   m_mapUserIds;
		
		int run_event_loop_i(int argc, ACE_TCHAR* argv[]);
		int init();
		int init_registry();
		ACE_CString get_bootstrap_filename();
		bool bootstrap_client(ACE_SOCK_STREAM& stream, bool bSandbox, ACE_CString& strSource);
		void end_event_loop_i();
		void term();
		bool connect_client_i(uid_t uid, u_short& uNewPort, ACE_CString& strSource);
		bool spawn_sandbox();
		bool spawn_client(uid_t uid, const ACE_CString& key, u_short& uNewPort, ACE_CString& strSource);

		void process_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::UShort src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs);
		bool access_check(ACE_HANDLE handle, const char* pszObject, ACE_UINT32 mode, bool& bAllowed);

		ACE_Configuration_Heap         m_registry;
		ACE_CString                    m_strRegistry;
		ACE_RW_Thread_Mutex            m_registry_lock;

		bool registry_open_section(ACE_HANDLE handle, ACE_InputCDR& request, ACE_Configuration_Section_Key& key, bool bAccessCheck = false);
		bool registry_open_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_Configuration_Section_Key& key, ACE_CString& strValue, bool bAccessCheck = false);
		void registry_key_exists(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_create_key(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_delete_key(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_enum_subkeys(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_value_type(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_get_string_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_get_uint_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_get_binary_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_set_string_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_set_uint_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_set_binary_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_enum_values(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_delete_value(ACE_HANDLE handle, ACE_InputCDR& request, ACE_OutputCDR& response);

		static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);
		static ACE_THR_FUNC_RETURN request_worker_fn(void* pParam);
	};
}

#endif // OOSERVER_ROOT_MANAGER_H_INCLUDED_
