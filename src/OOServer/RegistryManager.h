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
	class Manager : public OOBase::Server
	{
	public:
		Manager();
		virtual ~Manager();

		int run(OOBase::AllocatorInstance& allocator, const OOBase::LocalString& pszPipe);

		int on_start(const OOBase::LocalString& strDb, size_t nThreads, const OOBase::Table<OOBase::LocalString,OOBase::LocalString,OOBase::AllocatorInstance>& tabSettings);

#if defined(HAVE_UNISTD_H)
		int new_connection(int fd, uid_t uid);
#elif defined(_WIN32)
		int new_connection(const OOBase::LocalString& sid, OOBase::LocalString& strPipe);
#endif

	private:
		Manager(const Manager&);
		Manager& operator = (const Manager&);

		OOBase::SpinLock   m_lock;
		OOBase::Proactor*  m_proactor;
		OOBase::ThreadPool m_proactor_pool;
		OOBase::String     m_strDb;

		static int run_proactor(void* p);
		static int open_run_proactor(void* p);

		int open_database();

		bool connect_root(OOBase::AllocatorInstance& allocator, const OOBase::LocalString& strPipe);
	};
}

#endif // OOSERVER_REGISTRY_MANAGER_H_INCLUDED_
