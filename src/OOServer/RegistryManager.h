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

#ifndef OOSERVER_REGISTRY_MANAGER_H_INCLUDED_
#define OOSERVER_REGISTRY_MANAGER_H_INCLUDED_

#include "Protocol.h"

namespace Registry
{
	class Manager
	{
	public:
		Manager();
		virtual ~Manager();

		int run(const OOBase::LocalString& pszPipe);

		void on_root_closed();

#if defined(HAVE_UNISTD_H)
		int new_connection(int fd, uid_t uid, gid_t gid);
#elif defined(_WIN32)
		int new_connection(const OOBase::LocalString& sid, OOBase::LocalString& strPipe);
#endif

	private:
		Manager(const Manager&);
		Manager& operator = (const Manager&);

		OOBase::SpinLock   m_lock;
		OOBase::Proactor*  m_proactor;
		OOBase::ThreadPool m_proactor_pool;

		bool connect_root(const OOBase::LocalString& strPipe);
		static int run_proactor(void*);
	};
}

#endif // OOSERVER_REGISTRY_MANAGER_H_INCLUDED_
