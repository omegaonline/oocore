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

#include "Proactor.h"

#if defined(HAVE_EV_H)

#include "ProactorEv.h"

OOSvrBase::Ev::ProactorImpl::ProactorImpl() : m_pLoop(0)
{
	// We might be able improve performance by using a leader/follower pattern and more workers...
	void* TODO;

	// Create an ev loop
	m_pLoop = ev_loop_new(EVFLAG_AUTO | EVFLAG_NOENV);
	if (!m_pLoop)
		OOBase_CallCriticalFailure("ev_loop_new failed");

	// Init the async watcher so we can stop the loop
	ev_async_init(&m_stop_async,&async_stop);

	// Create a worker thread
	m_thread.run(&worker,this);
}

OOSvrBase::Ev::ProactorImpl::~ProactorImpl()
{
	// Signal the async watcher to terminate the thread
	ev_async_send(m_pLoop,&m_stop_async);

	// join() the thread
	m_thread.join();

	// Done with the loop
	ev_loop_destroy(m_pLoop);

	// Close any open sockets?
	void* POSIX_TODO;
}

int OOSvrBase::Ev::ProactorImpl::worker(void* param)
{
	return static_cast<ProactorImpl*>(param)->worker_i();
}

int OOSvrBase::Ev::ProactorImpl::worker_i()
{
	// Spin on the libev loop
	ev_loop(m_pLoop,0);

	return 0;
}

void OOSvrBase::Ev::ProactorImpl::async_stop(ev_loop* loop, ev_async* async, int)
{
	assert(async == &m_stop_async);
	assert(loop == m_pLoop);

	// Stop polling!
	ev_unloop(loop);
}

OOBase::Socket* OOSvrBase::Ev::ProactorImpl::accept_local(Acceptor* handler, const std::string& path, int* perr, SECURITY_ATTRIBUTES*)
{
	// path is a UNIX pipe name - e.g. /tmp/ooserverd 

	// create a socket on path and register with ev for accept, calling handler->on_accept() with each new socket

	void* POSIX_TODO;
	return 0;
}

OOSvrBase::AsyncSocket* OOSvrBase::Ev::ProactorImpl::attach_socket(IOHandler* handler, int* perr, OOBase::Socket* sock)
{
	// Cast to our known base
	OOBase::POSIX::Socket* pOrigSock = static_cast<OOBase::POSIX::Socket*>(sock);

	// Convert the underlying file handle in sock to non-blocking 
	// Insert into the libev implementation
	// and wrap in an AyncSocket implementation

	void* POSIX_TODO;
	return 0;
}

#endif // HAVE_EV_H
