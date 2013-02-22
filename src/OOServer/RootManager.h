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
//  ***** THIS IS A SECURE MODULE *****
//
//  It will be run as Administrator/setuid root
//
//  Therefore it needs to be SAFE AS HOUSES!
//
//  Do not include anything unnecessary
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_ROOT_MANAGER_H_INCLUDED_
#define OOSERVER_ROOT_MANAGER_H_INCLUDED_

#include "OOServer_Root.h"

#include "libdb/RegistryHive.h"

#include "MessageConnection.h"
#include "RootProcess.h"
#include "RootClientConn.h"
#include "RootRegistryConn.h"
#include "RootUserConn.h"
#include "Protocol.h"

namespace Root
{
	bool is_debug();

	class Manager :
			public Db::Manager,
			public OOBase::Server
	{
	public:
		Manager();

		int run(const OOBase::CmdArgs::results_t& cmd_args);

		bool get_config_arg(const char* name, OOBase::LocalString& val);

		bool find_user_process(ClientConnection* client);
		bool get_user_process(pid_t user_id, OOBase::RefPtr<UserConnection>& ptrUser);
		bool spawn_user_process(pid_t client_id, OOBase::RefPtr<RegistryConnection>& ptrRegistry);
		bool spawn_user_process(OOBase::RefPtr<ClientConnection>& ptrClient, OOBase::RefPtr<RegistryConnection>& ptrRegistry);

		OOBase::RefPtr<RegistryConnection> get_root_registry();
		void drop_registry_process(size_t id);
		void drop_user_process(pid_t id);
		void drop_client(pid_t id);

	private:
		Manager(const Manager&);
		Manager& operator = (const Manager&);

		OOBase::RWMutex                    m_lock;
		OOBase::Proactor*                  m_proactor;
		OOBase::ThreadPool                 m_proactor_pool;

		// Init and run members
		bool load_config(const OOBase::CmdArgs::results_t& cmd_args);
		static int run_proactor(void* param);

		// Configuration members
		OOBase::Table<OOBase::String,OOBase::String> m_config_args;

		// Registry handling
		OOBase::HandleTable<size_t,OOBase::RefPtr<RegistryConnection> > m_registry_processes;

		bool start_system_registry(OOBase::AllocatorInstance& allocator);
		bool spawn_user_registry(OOBase::RefPtr<ClientConnection>& ptrClient);
		bool get_registry_hive(const uid_t& uid, OOBase::LocalString strSysDir, OOBase::LocalString strUsersDir, OOBase::LocalString& strHive);

		// User process handling
		OOBase::HashTable<pid_t,OOBase::RefPtr<UserConnection> > m_user_processes;

		bool platform_spawn(OOBase::LocalString strBinPath, const uid_t& uid, const char* session_id, const OOBase::Environment::env_table_t& tabEnv, OOBase::SmartPtr<Process>& ptrProcess, OOBase::RefPtr<OOBase::AsyncSocket>& ptrSocket, bool& bAgain);
		bool spawn_sandbox_process(OOBase::AllocatorInstance& allocator);
		bool get_our_uid(uid_t& uid, OOBase::LocalString& strUName);
		bool get_sandbox_uid(const OOBase::LocalString& strUName, uid_t& uid, bool& bAgain);

		// Client handling members
		SECURITY_ATTRIBUTES              m_sa;
#if defined(_WIN32)
		OOBase::Win32::sec_descript_t    m_sd;
#endif
		OOBase::RefPtr<OOBase::Acceptor> m_client_acceptor;
		OOBase::HashTable<pid_t,OOBase::RefPtr<ClientConnection> > m_clients;

		bool start_client_acceptor(OOBase::AllocatorInstance& allocator);
		static void accept_client(void* pThis, OOBase::AsyncSocket* pSocket, int err);

		// Service handling
		bool start_services();
		bool stop_services();
		void start_service(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void stop_service(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void service_is_running(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void service_list_running(Omega::uint32_t channel_id, OOBase::CDRStream& response);

		// Registry members
		OOBase::SmartPtr<Db::Hive> m_registry;

		Db::hive_errors registry_open_key(Omega::int64_t& uKey, const OOBase::LocalString& strSubKey, Omega::uint32_t channel_id);
		bool registry_access_check(const char* pszDb, Omega::uint32_t channel_id, Db::access_rights_t access_mask, int& err);
		OOServer::RootErrCode_t registry_open_hive(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::SmartPtr<Db::Hive>& ptrHive, Omega::int64_t& uKey, Omega::byte_t& nType);
		OOServer::RootErrCode_t registry_open_link(Omega::uint32_t channel_id, const OOBase::LocalString& strLink, OOBase::LocalString& strSubKey, Omega::byte_t& nType, OOBase::SmartPtr<Db::Hive>& ptrHive);
		void registry_open_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_delete_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_enum_subkeys(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_value_exists(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_get_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_set_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_enum_values(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_delete_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
	};
}

#endif // OOSERVER_ROOT_MANAGER_H_INCLUDED_

