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

#ifndef OOSERVER_USER_MANAGER_H_INCLUDED_
#define OOSERVER_USER_MANAGER_H_INCLUDED_

#include "UserRootConn.h"
#include "Protocol.h"
#include "UserProcess.h"

namespace User
{
	class Manager : public OOBase::Server
	{
		friend class RootConnection;

	public:
		Manager();
		virtual ~Manager();

		static Manager* instance() { return s_instance; }

		int run(const OOBase::LocalString& pszPipe);
		void sendrecv_root(const OOBase::CDRStream& request, OOBase::CDRStream* response, Omega::TypeInfo::MethodAttributes_t attribs);
		void close_socket(Omega::uint32_t id);
		
		void get_root_config_arg(const char* key, Omega::string_t& strValue);
		Process* exec(const Omega::string_t& strProcess, const Omega::string_t& strWorkingDir, bool is_host_process, const OOBase::Environment::env_table_t& tabEnv);
		
	private:
		static Manager* s_instance; //  This is a poor-mans singleton

		OOBase::RWMutex                      m_lock;
		OOBase::Proactor*                    m_proactor;
		OOBase::ThreadPool                   m_thread_pool;
		bool                                 m_bIsSandbox;
		OOBase::RefPtr<User::RootConnection> m_root_connection;

		bool connect_root(const OOBase::LocalString& strPipe);
		int start(OOBase::RefPtr<OOBase::AsyncSocket>& ptrUserSocket, OOBase::RefPtr<OOBase::AsyncSocket>& ptrRootSocket, OOBase::CDRStream& stream);

		static int run_proactor(void* param);
		static int do_bootstrap(void* param);
		static int do_handle_events(void* param);

		bool bootstrap();
		void do_quit();

		// Remote channel handling
		Omega::Remoting::IChannel* open_remote_channel_i(const Omega::string_t& strEndpoint);

		// Service management (only for sandbox)
		struct ServiceEntry
		{
			OOBase::String strName;
			OTL::ObjectPtr<Omega::System::IService> ptrService;
		};
		OOBase::Stack<ServiceEntry> m_mapServices;

		bool notify_started();
		void start_service(OOBase::CDRStream& request, OOBase::CDRStream* response);
		OOServer::RootErrCode_t stop_all_services();
		void stop_all_services(OOBase::CDRStream& response);
		void stop_service(OOBase::CDRStream& request, OOBase::CDRStream& response);
		void service_is_running(OOBase::CDRStream& request, OOBase::CDRStream& response);
		void list_services(OOBase::CDRStream& response);
	};
}

#endif // OOSERVER_USER_MANAGER_H_INCLUDED_
