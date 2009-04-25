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

#include "OOServer_Root.h"
#include "MessageConnection.h"
#include "ClientAcceptor.h"
#include "RegistryHive.h"
#include "SpawnedProcess.h"

namespace Root
{
	typedef OOBase::Singleton<OOSvrBase::Proactor> Proactor;

	class Manager :
		public MessageHandler
	{
	public:
		Manager();
		virtual ~Manager();

		bool install(int argc, char* argv[]);
		bool uninstall();
		int run(int argc, char* argv[]);

		int registry_access_check(Omega::uint32_t channel_id);
		std::string get_user_pipe(OOBase::LocalSocket::uid_t uid);

	private:
		Manager(const Manager&) : MessageHandler() {}
		Manager& operator = (const Manager&) { return *this; }

		// Init and run members
		bool init();
		bool get_db_fname();
		bool init_database();
		void wait_for_quit();
		
		// Installation members
		bool platform_install(int argc, char* argv[]);
		bool platform_uninstall();
		bool install_sandbox(int argc, char* argv[]);
		bool uninstall_sandbox();
		bool secure_file(const std::string& strFilename);

		// I/O members
		OOBase::RWMutex m_lock;
		Omega::uint32_t m_sandbox_channel;
		ClientAcceptor  m_client_acceptor;

		// Spawned process members
		struct UserProcess
		{
			std::string                      strPipe;
			OOBase::SmartPtr<SpawnedProcess> ptrSpawn;
			OOBase::SmartPtr<RegistryHive>   ptrRegistry;
		};
		std::map<Omega::uint32_t,UserProcess> m_mapUserProcesses;
		
		Omega::uint32_t spawn_user(OOBase::LocalSocket::uid_t uid, OOBase::SmartPtr<RegistryHive> ptrRegistry, std::string& strPipe);
		OOBase::SmartPtr<SpawnedProcess> platform_spawn(OOBase::LocalSocket::uid_t uid, std::string& strPipe, Omega::uint32_t& channel_id, OOBase::SmartPtr<MessageConnection>& ptrMC);
		Omega::uint32_t bootstrap_user(OOBase::Socket* pSocket, OOBase::SmartPtr<MessageConnection>& ptrMC, std::string& strPipe);

		// Message handling members
		virtual bool can_route(Omega::uint32_t src_channel, Omega::uint32_t dest_channel);
		virtual void on_channel_closed(Omega::uint32_t channel);
		virtual void process_request(OOBase::CDRStream& request, Omega::uint32_t seq_no, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs);

		// Registry members
		OOBase::SmartPtr<Db::Database> m_db;
		OOBase::SmartPtr<RegistryHive> m_registry;
		std::string                    m_strRegistry;

		int registry_parse_subkey(const Omega::int64_t& uKey, Omega::uint32_t& channel_id, const std::string& strSubKey, bool& bCurrent, OOBase::SmartPtr<RegistryHive>& ptrHive);
		int registry_open_hive(Omega::uint32_t& channel_id, OOBase::CDRStream& request, OOBase::SmartPtr<RegistryHive>& ptrHive, Omega::int64_t& uKey);
		int registry_open_hive(Omega::uint32_t& channel_id, OOBase::CDRStream& request, OOBase::SmartPtr<RegistryHive>& ptrHive, Omega::int64_t& uKey, bool& bCurrent);
		void registry_key_exists(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_create_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_delete_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_enum_subkeys(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_value_type(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_get_string_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_get_int_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_get_binary_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_set_string_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_set_int_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_set_binary_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_set_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_set_value_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_get_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_get_value_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_enum_values(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_delete_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
	};
}

#endif // OOSERVER_ROOT_MANAGER_H_INCLUDED_

