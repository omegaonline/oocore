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
#include "MessageConnection.h"
#include "RegistryHive.h"
#include "SpawnedProcess.h"

#if defined(_WIN32) && !defined(__MINGW32__)
#define APPNAME "OOServer"
#else
#define APPNAME "ooserverd"
#endif

namespace Root
{
	typedef OOBase::Singleton<OOSvrBase::Proactor,Root::Module> Proactor;

	class Manager :
			public OOServer::MessageHandler,
			public Registry::Manager,
			public OOSvrBase::Service
	{
	public:
		Manager();
		virtual ~Manager();

		int run(const OOBase::CmdArgs::results_t& cmd_args);

	private:
		Manager(const Manager&);
		Manager& operator = (const Manager&);

		OOBase::RWMutex                       m_lock;
		OOBase::ThreadPool                    m_proactor_pool;

		// Init and run members
		bool load_config(const OOBase::CmdArgs::results_t& cmd_args);
		bool load_config_file(const char* pszFile);
		bool init_database();
		bool spawn_sandbox();
		bool wait_to_quit();
		static int run_proactor(void* param);

		// Configuration members
		OOBase::Table<OOBase::String,OOBase::String> m_config_args;
		bool                                         m_bUnsafe;

		// Client handling members
		SECURITY_ATTRIBUTES                 m_sa;
		OOBase::RefPtr<OOSvrBase::Acceptor> m_client_acceptor;
#if defined(_WIN32)
		OOSvrBase::Win32::sec_descript_t    m_sd;
#endif
		bool start_client_acceptor();
		static void accept_client(void* pThis, OOSvrBase::AsyncLocalSocket* pSocket, int err);
		void accept_client_i(OOSvrBase::AsyncLocalSocket* pSocket, int err);

		// Spawned process members
		struct UserProcess
		{
			OOBase::String                   strPipe;
			OOBase::SmartPtr<SpawnedProcess> ptrSpawn;
			OOBase::SmartPtr<Registry::Hive> ptrRegistry;
		};
		Omega::uint32_t                       m_sandbox_channel;

		typedef OOBase::HashTable<Omega::uint32_t,UserProcess> mapUserProcessesType;
		mapUserProcessesType m_mapUserProcesses;

		OOBase::SmartPtr<SpawnedProcess> platform_spawn(OOSvrBase::AsyncLocalSocket::uid_t uid, const char* session_id, OOBase::String& strPipe, Omega::uint32_t& channel_id, OOBase::RefPtr<OOServer::MessageConnection>& ptrMC, bool& bAgain);
		Omega::uint32_t bootstrap_user(OOBase::RefPtr<OOSvrBase::AsyncLocalSocket>& ptrSocket, OOBase::RefPtr<OOServer::MessageConnection>& ptrMC, OOBase::String& strPipe);
		Omega::uint32_t spawn_user(OOSvrBase::AsyncLocalSocket::uid_t uid, const char* session_id, const OOBase::SmartPtr<Registry::Hive>& ptrRegistry, OOBase::String& strPipe, bool& bAgain);
		bool get_user_process(OOSvrBase::AsyncLocalSocket::uid_t& uid, const char* session_id, UserProcess& user_process);
		bool get_our_uid(OOSvrBase::AsyncLocalSocket::uid_t& uid, OOBase::LocalString& strUName);
		bool get_sandbox_uid(const OOBase::String& strUName, OOSvrBase::AsyncLocalSocket::uid_t& uid, bool& bAgain);

		// Message handling members
		virtual bool can_route(Omega::uint32_t src_channel, Omega::uint32_t dest_channel);
		virtual void on_channel_closed(Omega::uint32_t channel);
		virtual void process_request(OOBase::CDRStream& request, Omega::uint32_t seq_no, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs);

		// Registry members
		OOBase::SmartPtr<Registry::Hive> m_registry;
		OOBase::SmartPtr<Registry::Hive> m_registry_sandbox;

		int registry_access_check(const char* pszDb, Omega::uint32_t channel_id, Registry::Hive::access_rights_t access_mask);

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

		// Services members
	public:
		struct ControlledObject
		{
			virtual ~ControlledObject() {}
		};

		struct Socket : public ControlledObject
		{
			virtual int recv(Omega::uint32_t lenBytes, Omega::bool_t bRecvAll) = 0;
			virtual int send(OOBase::Buffer* buffer, Omega::bool_t bReliable) = 0;
		};

		Omega::uint32_t add_socket(Omega::uint32_t acceptor_id, Socket* pSocket);
		void remove_socket(Omega::uint32_t id);
		void remove_listener(Omega::uint32_t id);

		io_result::type sendrecv_sandbox(const OOBase::CDRStream& request, OOBase::CDRStream* response, const OOBase::timeval_t* deadline, Omega::uint16_t attribs);

	private:
		OOBase::HandleTable<Omega::uint32_t,OOBase::SmartPtr<Socket> >         m_mapSockets;
		OOBase::HashTable<Omega::uint32_t,OOBase::SmartPtr<ControlledObject> > m_mapListeners;
		
		void stop_services();
		int create_service_listener(Omega::uint32_t id, const OOBase::LocalString& strProtocol, const OOBase::LocalString& strAddress, const OOBase::LocalString& strPort);
		void services_start(Omega::uint32_t channel_id, OOBase::CDRStream& response);
		void get_service_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void listen_socket(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void socket_recv(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void socket_send(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void socket_close(Omega::uint32_t channel_id, OOBase::CDRStream& request);
	};
}

#endif // OOSERVER_ROOT_MANAGER_H_INCLUDED_

