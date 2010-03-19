///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOSvrBase, the Omega Online Base library.
//
// OOSvrBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOSvrBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOSVRBASE_PROACTOR_EV_H_INCLUDED_
#define OOSVRBASE_PROACTOR_EV_H_INCLUDED_

#if !defined(OOSVRBASE_PROACTOR_H_INCLUDED_)
#error include "Proactor.h" instead
#endif

#if !defined(HAVE_EV_H)
#error Includes have got confused, check Proactor.h
#endif

#include <queue>
#include <vector>

#include "Thread.h"
#include "Condition.h"

namespace OOSvrBase
{
	namespace Ev
	{
		class ProactorImpl : public detail::ProactorImpl
		{
		public:
			ProactorImpl();
			~ProactorImpl();

			OOBase::Socket* accept_local(Acceptor* handler, const std::string& path, int* perr, SECURITY_ATTRIBUTES*);

			AsyncSocket* attach_socket(IOHandler* handler, int* perr, OOBase::Socket* sock);

			struct io_watcher : public ev_io
			{
				void* param;
				void (*callback)(void*);
			};

			int add_watcher(io_watcher*& pNew, int fd, int events, void* param, void (*callback)(void*));
			int start_watcher(io_watcher* watcher);
			int stop_watcher(io_watcher* watcher);
			int remove_watcher(io_watcher* watcher);

		private:
			struct io_info
			{
				io_watcher* watcher;
				int         op; // 0-start,1-stop,2-delete
			};

			// The following vars all use this lock
			OOBase::Mutex            m_ev_lock;
			ev_loop*                 m_pLoop;
			std::queue<io_watcher*>* m_pIOQueue;
			bool                     m_bAsyncTriggered;
						
			// The following vars all use this lock
			OOBase::SpinLock    m_lock;
			bool                m_bStop;
			std::queue<io_info> m_update_queue;

			// The following vars don't...
			std::vector<OOBase::SmartPtr<OOBase::Thread> > m_workers;
			ev_async                                       m_alert;

			static int worker(void* param);
			int worker_i();

			static void on_alert(ev_loop*, ev_async* watcher, int);
			static void on_io(ev_loop*, ev_io* watcher, int events);
			void on_io_i(io_watcher* watcher, int events);
		};
	}
}

#endif // OOSVRBASE_PROACTOR_EV_H_INCLUDED_
