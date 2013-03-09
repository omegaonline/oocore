///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2013 Rick Taylor
//
// This file is part of OOSvrReg, the Omega Online registry server.
//
// OOSvrReg is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrReg is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOSvrReg.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOSERVER_REGISTRY_MANAGER_H_INCLUDED_
#define OOSERVER_REGISTRY_MANAGER_H_INCLUDED_

#include "Protocol.h"

#if defined(_WIN32)
typedef OOBase::Win32::SmartHandle uid_t;
#endif

namespace Registry
{
	class Manager : public OOBase::Server
	{
		friend class RootConnection;

	public:
		Manager();
		virtual ~Manager();

		int run(const OOBase::LocalString& pszPipe);

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

		bool connect_root(const OOBase::LocalString& strPipe);

		int on_start(const OOBase::LocalString& strDb, size_t nThreads, const OOBase::Table<OOBase::LocalString,OOBase::LocalString,OOBase::AllocatorInstance>& tabSettings);

		int new_connection(OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket, const uid_t& uid);
	};
}

#endif // OOSERVER_REGISTRY_MANAGER_H_INCLUDED_
