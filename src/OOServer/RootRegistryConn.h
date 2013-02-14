///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2013 Rick Taylor
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

#ifndef OOSERVER_ROOT_REGISTRY_CONN_H_INCLUDED_
#define OOSERVER_ROOT_REGISTRY_CONN_H_INCLUDED_

#include "RootUserConn.h"

namespace Root
{
	class RegistryConnection : public OOBase::RefCounted
	{
	public:
		RegistryConnection(Manager* pManager, OOBase::SmartPtr<Process>& ptrProcess, OOBase::RefPtr<OOBase::AsyncSocket>& ptrSocket);

		bool start(size_t id);
		bool start(OOBase::CDRStream& stream, size_t id);

		bool same_user(const uid_t& uid) const;

		bool start_user(const uid_t& uid, OOBase::RefPtr<UserConnection>& ptrUser);
		bool get_environment(const char* key, OOBase::Environment::env_table_t& tabEnv);

	private:
		Manager*                            m_pManager;
		OOBase::SmartPtr<Process>           m_ptrProcess;
		OOBase::RefPtr<OOBase::AsyncSocket> m_ptrSocket;
		size_t                              m_id;

		void on_started(OOBase::CDRStream& stream, int err);

#if defined(HAVE_UNISTD_H)
		void on_sent_msg(OOBase::Buffer* data, OOBase::Buffer* ctl, int err);
#endif
	};
}

#endif // OOSERVER_ROOT_REGISTRY_CONN_H_INCLUDED_
