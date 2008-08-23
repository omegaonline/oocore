///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

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

#include "./OOServer_Root.h"
#include "./MessageConnection.h"
#include "./RootHttp.h"
#include "./RegistryHive.h"

namespace Root
{
	class SpawnedProcess;

	class Manager :
		public MessageHandler
	{
	public:
		static int run(int argc, ACE_TCHAR* argv[]);
		static void end();
		static ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Thread_Mutex> get_registry();
		static bool install(int argc, ACE_TCHAR* argv[]);
		static bool uninstall();
		static int registry_access_check(ACE_CDR::ULong channel_id);

		static bool call_async_function(void (*pfnCall)(void*,ACE_InputCDR&), void* pParam, const ACE_Message_Block* mb = 0);

	private:
		friend class HttpAcceptor;
		friend class MessagePipeSingleAsyncAcceptor<Manager>;
		friend class ACE_Singleton<Manager,ACE_Recursive_Thread_Mutex>;
		typedef ACE_Singleton<Manager, ACE_Recursive_Thread_Mutex> ROOT_MANAGER;

		Manager();
		Manager(const Manager&) : MessageHandler() {}
		virtual ~Manager();
		Manager& operator = (const Manager&) { return *this; }

		ACE_RW_Thread_Mutex  m_lock;

		int run_event_loop_i(int argc, ACE_TCHAR* argv[]);
		bool init();
		int init_database();
		void end_event_loop_i();

		struct UserProcess
		{
			ACE_CString                                          strPipe;
			SpawnedProcess*                                      pSpawn;
			ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Thread_Mutex> ptrRegistry;
		};
		std::map<ACE_CDR::ULong,UserProcess>    m_mapUserProcesses;
		MessagePipeSingleAsyncAcceptor<Manager> m_client_acceptor;
		ACE_CDR::ULong                          m_sandbox_channel;

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
		int on_accept(const ACE_Refcounted_Auto_Ptr<ACE_SPIPE_Stream,ACE_Thread_Mutex>& pipe);
#else
		int on_accept(const ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex>& pipe);
#endif

		virtual bool can_route(ACE_CDR::ULong src_channel, ACE_CDR::ULong dest_channel);
		virtual void on_channel_closed(ACE_CDR::ULong channel);

		int process_client_connects();
		ACE_CDR::ULong spawn_user(user_id_type uid, ACE_CString& strPipe, ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Thread_Mutex> ptrRegistry);
		ACE_CString bootstrap_user(const ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex>& pipe);
		bool connect_client(user_id_type uid, ACE_CString& strPipe);
		void close_users();
		ACE_CDR::ULong connect_user(MessagePipeAcceptor& acceptor, SpawnedProcess* pSpawn, ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Thread_Mutex> ptrRegistry, ACE_CString& strPipe);

		void process_request(ACE_InputCDR& request, ACE_CDR::ULong seq_no, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs);

		ACE_Refcounted_Auto_Ptr<Db::Database,ACE_Thread_Mutex> m_db;
		ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Thread_Mutex> m_registry;
		ACE_CString                                            m_strRegistry;

		int registry_parse_subkey(const ACE_INT64& uKey, ACE_CDR::ULong& channel_id, const ACE_CString& strSubKey, bool& bCurrent, ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Thread_Mutex>& ptrHive);
		int registry_open_hive(ACE_CDR::ULong& channel_id, ACE_InputCDR& request, ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Thread_Mutex>& ptrHive, ACE_INT64& uKey);
		int registry_open_hive(ACE_CDR::ULong& channel_id, ACE_InputCDR& request, ACE_Refcounted_Auto_Ptr<RegistryHive,ACE_Thread_Mutex>& ptrHive, ACE_INT64& uKey, bool& bCurrent);
		void registry_key_exists(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_create_key(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_delete_key(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_enum_subkeys(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_value_type(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_get_string_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_get_int_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_get_binary_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_set_string_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_set_int_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_set_binary_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_set_description(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_set_value_description(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_get_description(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_get_value_description(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_enum_values(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_delete_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);

		HttpAcceptor* m_http_acceptor;
		bool sendrecv_sandbox(const ACE_OutputCDR& request, ACE_CDR::ULong attribs, ACE_InputCDR*& response);
	};
}

#endif // OOSERVER_ROOT_MANAGER_H_INCLUDED_

