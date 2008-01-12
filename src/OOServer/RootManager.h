///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
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

namespace Root
{
	class SpawnedProcess;

	class Manager :
		public MessageHandler
	{
	public:
		static int run(int argc, wchar_t* argv[]);
		static void end();
		static ACE_Configuration_Heap& get_registry();
		static bool install(int argc, wchar_t* argv[]);
		static bool uninstall();

	private:
		friend class MessagePipeSingleAsyncAcceptor<Manager>;
		friend class ACE_Singleton<Manager,ACE_Thread_Mutex>;
		typedef ACE_Singleton<Manager, ACE_Thread_Mutex> ROOT_MANAGER;

		Manager();
		Manager(const Manager&) : MessageHandler() {}
		virtual ~Manager();
		Manager& operator = (const Manager&) { return *this; }

		ACE_RW_Thread_Mutex  m_lock;

		int run_event_loop_i(int argc, wchar_t* argv[]);
		bool init();
		int init_registry();
		void end_event_loop_i();

		struct UserProcess
		{
			ACE_WString     strPipe;
			SpawnedProcess* pSpawn;
			bool            bPrimary;	// This is the instance that holds the user registry
		};
		std::map<ACE_CDR::ULong,UserProcess>    m_mapUserProcesses;
		MessagePipeSingleAsyncAcceptor<Manager> m_client_acceptor;

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
		int on_accept(ACE_SPIPE_Stream& pipe);
#else
		int on_accept(MessagePipe& pipe);
#endif
		int process_client_connects();
		ACE_CDR::ULong spawn_user(user_id_type uid, ACE_CDR::ULong nUserChannel, ACE_WString& strPipe);
		ACE_WString bootstrap_user(MessagePipe& pipe, ACE_CDR::ULong nUserChannel);
		bool connect_client(user_id_type uid, ACE_WString& strPipe);
		void close_users();

		void process_request(ACE_InputCDR& request, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs);
		bool access_check(ACE_CDR::ULong channel, const wchar_t* pszObject, ACE_UINT32 mode, bool& bAllowed);

		ACE_Configuration_Heap         m_registry;
		ACE_WString                    m_strRegistry;
		ACE_RW_Thread_Mutex            m_registry_lock;
		ACE_CDR::ULong                 m_sandbox_channel;

		bool registry_open_section(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_Configuration_Section_Key& key, bool bAccessCheck = false);
		bool registry_open_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_Configuration_Section_Key& key, ACE_WString& strValue, bool bAccessCheck = false);
		void registry_key_exists(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_create_key(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_delete_key(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_enum_subkeys(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_value_type(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_get_string_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_get_uint_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_get_binary_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_set_string_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_set_uint_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_set_binary_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_enum_values(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);
		void registry_delete_value(ACE_CDR::ULong channel_id, ACE_InputCDR& request, ACE_OutputCDR& response);

		static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);
		static ACE_THR_FUNC_RETURN request_worker_fn(void* pParam);
	};
}

#endif // OOSERVER_ROOT_MANAGER_H_INCLUDED_

