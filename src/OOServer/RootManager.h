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

#include "../../libdb/RegistryHive.h"

#include "MessageConnection.h"
#include "RootProcess.h"

#if defined(_WIN32) && !defined(__MINGW32__)
#define APPNAME "OOServer"
#else
#define APPNAME "ooserverd"
#endif

namespace Root
{
	bool is_debug();

	class Manager :
			public OOServer::MessageHandler,
			public Db::Manager,
			public OOBase::Service
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
		bool load_config_i(const OOBase::CmdArgs::results_t& cmd_args);
		bool load_config_file(const char* pszFile);
		bool init_database();
		bool spawn_sandbox();
		bool wait_to_quit();
		static int run_proactor(void* param);

		// Configuration members
		OOBase::Table<OOBase::String,OOBase::String> m_config_args;
		bool get_config_arg(const char* name, OOBase::String& val);

		// Client handling members
		SECURITY_ATTRIBUTES                 m_sa;
		OOBase::RefPtr<OOSvrBase::Acceptor> m_client_acceptor;
#if defined(_WIN32)
		OOBase::Win32::sec_descript_t       m_sd;
#endif
		bool start_client_acceptor();
		static void accept_client(void* pThis, OOSvrBase::AsyncLocalSocket* pSocket, int err);
		void accept_client_i(OOBase::RefPtr<OOSvrBase::AsyncLocalSocket>& ptrSocket, int err);

		// Spawned process members
		struct UserProcess
		{
			OOBase::String                   m_strPipe;
			OOBase::SmartPtr<Process>        m_ptrProcess;
			OOBase::SmartPtr<Db::Hive> m_ptrRegistry;
		};
		Omega::uint32_t                      m_sandbox_channel;

		typedef OOBase::HashTable<Omega::uint32_t,UserProcess> mapUserProcessesType;
		mapUserProcessesType                                   m_mapUserProcesses;

		OOBase::SmartPtr<Process> platform_spawn(OOSvrBase::AsyncLocalSocket::uid_t uid, const char* session_id, OOBase::String& strPipe, Omega::uint32_t& channel_id, OOBase::RefPtr<OOServer::MessageConnection>& ptrMC, bool& bAgain);
		Omega::uint32_t bootstrap_user(OOBase::RefPtr<OOSvrBase::AsyncLocalSocket>& ptrSocket, OOBase::RefPtr<OOServer::MessageConnection>& ptrMC, OOBase::String& strPipe);
		Omega::uint32_t spawn_user(OOSvrBase::AsyncLocalSocket::uid_t uid, const char* session_id, const OOBase::SmartPtr<Db::Hive>& ptrRegistry, OOBase::String& strPipe, bool& bAgain);
		bool get_user_process(OOSvrBase::AsyncLocalSocket::uid_t& uid, const OOBase::LocalString& session_id, UserProcess& user_process);
		bool get_our_uid(OOSvrBase::AsyncLocalSocket::uid_t& uid, OOBase::LocalString& strUName);
		bool get_sandbox_uid(const OOBase::String& strUName, OOSvrBase::AsyncLocalSocket::uid_t& uid, bool& bAgain);

		// Message handling members
		virtual bool can_route(Omega::uint32_t src_channel, Omega::uint32_t dest_channel);
		virtual void on_channel_closed(Omega::uint32_t channel);
		virtual void process_request(OOBase::CDRStream& request, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, const OOBase::Timeout& timeout, Omega::uint32_t attribs);

		// Registry members
		OOBase::SmartPtr<Db::Hive> m_registry;
		OOBase::SmartPtr<Db::Hive> m_registry_sandbox;

		int registry_access_check(const char* pszDb, Omega::uint32_t channel_id, Db::Hive::access_rights_t access_mask);

		int registry_open_hive(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::SmartPtr<Db::Hive>& ptrHive, Omega::int64_t& uKey, Omega::byte_t& nType);
		void registry_key_exists(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_create_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_delete_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_enum_subkeys(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_value_exists(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_get_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_set_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_enum_values(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_delete_value(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		void registry_open_mirror_key(Omega::uint32_t channel_id, OOBase::CDRStream& request, OOBase::CDRStream& response);
		int registry_open_key(Omega::int64_t uParent, Omega::int64_t& uKey, const char* pszSubKey, Omega::uint32_t channel_id);
		int registry_open_link(Omega::uint32_t channel_id, const OOBase::LocalString& strLink, OOBase::LocalString& strSubKey, Omega::byte_t& nType, OOBase::SmartPtr<Db::Hive>& ptrHive);

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

		io_result::type sendrecv_sandbox(const OOBase::CDRStream& request, OOBase::CDRStream* response, Omega::uint16_t attribs);

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

	typedef OOBase::Singleton<OOSvrBase::Proactor,Manager> Proactor;
}

#endif // OOSERVER_ROOT_MANAGER_H_INCLUDED_

