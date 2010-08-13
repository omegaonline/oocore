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
//  Do not include anything unecessary
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
	typedef OOBase::Singleton<OOSvrBase::Proactor,Root::Module> Proactor;

	class Manager :
			public OOServer::MessageHandler,
			public Registry::Manager,
			public OOSvrBase::Service
	{
	public:
		Manager(const std::map<std::string,std::string>& args);
		virtual ~Manager();

		int run();

		void accept_client(OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket>& ptrSocket);

	private:
		Manager(const Manager&);
		Manager& operator = (const Manager&);

		// Init and run members
		bool load_config();
		bool load_config_file(const std::string& strFile);
		bool init_database();
		
		// Configuration members
		std::map<std::string,std::string> m_cmd_args;
		std::map<std::string,std::string> m_config_args;

		// I/O members
		OOBase::RWMutex m_lock;
		Omega::uint32_t m_sandbox_channel;
		ClientAcceptor  m_client_acceptor;

		// Spawned process members
		struct UserProcess
		{
			std::string                      strPipe;
			OOBase::SmartPtr<SpawnedProcess> ptrSpawn;
			OOBase::SmartPtr<Registry::Hive> ptrRegistry;
		};
		std::map<Omega::uint32_t,UserProcess> m_mapUserProcesses;

		OOBase::SmartPtr<SpawnedProcess> platform_spawn(OOSvrBase::AsyncLocalSocket::uid_t uid, std::string& strPipe, Omega::uint32_t& channel_id, OOBase::SmartPtr<OOServer::MessageConnection>& ptrMC);
		Omega::uint32_t bootstrap_user(OOBase::SmartPtr<OOSvrBase::AsyncSocket>& ptrSocket, OOBase::SmartPtr<OOServer::MessageConnection>& ptrMC, std::string& strPipe);
		Omega::uint32_t spawn_user(OOSvrBase::AsyncLocalSocket::uid_t uid, OOBase::SmartPtr<Registry::Hive> ptrRegistry, std::string& strPipe);
		bool get_user_process(OOSvrBase::AsyncLocalSocket::uid_t uid, UserProcess& user_process);

		// Message handling members
		virtual bool can_route(Omega::uint32_t src_channel, Omega::uint32_t dest_channel);
		virtual void on_channel_closed(Omega::uint32_t channel);
		virtual void process_request(OOBase::CDRStream& request, Omega::uint32_t seq_no, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs);

		// Registry members
		OOBase::SmartPtr<Registry::Hive> m_registry;
		OOBase::SmartPtr<Registry::Hive> m_registry_sandbox;

		int registry_access_check(const std::string& strdb, Omega::uint32_t channel_id, Registry::Hive::access_rights_t access_mask);

		int registry_open_hive(Omega::uint32_t& channel_id, OOBase::CDRStream& request, OOBase::SmartPtr<Registry::Hive>& ptrHive, Omega::int64_t& uKey);
		int registry_open_hive(Omega::uint32_t& channel_id, OOBase::CDRStream& request, OOBase::SmartPtr<Registry::Hive>& ptrHive, Omega::int64_t& uKey, Omega::byte_t& nType);
		void registry_key_exists(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_create_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_delete_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_enum_subkeys(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_value_exists(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_get_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_set_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_set_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_set_value_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_get_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_get_value_description(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_enum_values(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_delete_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_open_mirror_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
	};
}

#endif // OOSERVER_ROOT_MANAGER_H_INCLUDED_

