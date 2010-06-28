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

#include <algorithm>

#if defined(HAVE_EV_H)

#include "ProactorEv.h"
#include "PosixSocket.h"

#if defined(HAVE_SYS_UN_H)
#include <sys/un.h>
#endif /* HAVE_SYS_UN_H */

namespace
{
	class AsyncSocket : public OOSvrBase::AsyncSocket
	{
	public:
		AsyncSocket(OOSvrBase::Ev::ProactorImpl* pProactor) :
				m_proactor(pProactor),
				m_read_watcher(0),
				m_write_watcher(0),
				m_handler(0),
				m_current_write(0)
		{
			m_current_read.m_buffer = 0;
		}

		int bind(OOSvrBase::IOHandler* handler, int fd);

		int read(OOBase::Buffer* buffer, size_t len = 0);
		int write(OOBase::Buffer* buffer);
		void close();

	private:
		virtual ~AsyncSocket();

		struct AsyncRead
		{
			OOBase::Buffer* m_buffer;
			size_t          m_to_read;
		};

		OOBase::Mutex                            m_lock;
		OOSvrBase::Ev::ProactorImpl*             m_proactor;
		OOSvrBase::Ev::ProactorImpl::io_watcher* m_read_watcher;
		OOSvrBase::Ev::ProactorImpl::io_watcher* m_write_watcher;
		OOSvrBase::IOHandler*                    m_handler;
		std::deque<AsyncRead>                    m_async_reads;
		std::deque<OOBase::Buffer*>              m_async_writes;
		AsyncRead                                m_current_read;
		OOBase::Buffer*                          m_current_write;

		static void on_read(void* param);
		static void on_write(void* param);

		bool do_read(size_t to_read);
		int read_next();

		bool do_write();
		int write_next();
	};

	template <typename SocketType>
	class AcceptSocket : public OOBase::Socket
	{
	public:
		AcceptSocket(OOSvrBase::Ev::ProactorImpl* pProactor, const std::string& path) :
				m_proactor(pProactor),
				m_watcher(0),
				m_handler(0),
				m_closing(false),
				m_path(path)
		{}

		int init(OOSvrBase::Acceptor* handler, int fd);

		int send(const void* /*buf*/, size_t /*len*/, const OOBase::timeval_t* /*timeout*/ = 0)
		{
			return EINVAL;
		}

		size_t recv(void* /*buf*/, size_t /*len*/, int* perr, const OOBase::timeval_t* /*timeout*/ = 0)
		{
			*perr = EINVAL;
			return 0;
		}

		void close();

	private:
		OOBase::Condition::Mutex                 m_lock;
		OOSvrBase::Ev::ProactorImpl*             m_proactor;
		OOSvrBase::Ev::ProactorImpl::io_watcher* m_watcher;
		OOSvrBase::Acceptor*                     m_handler;
		OOBase::Condition                        m_close_cond;
		bool                                     m_closing;
		std::string                              m_path;

		static void on_accept(void* param);
		void on_accept_i();
	};

	AsyncSocket::~AsyncSocket()
	{
		try
		{
			close();
		}
		catch (...)
		{}
	}

	int AsyncSocket::bind(OOSvrBase::IOHandler* handler, int fd)
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		// Add our watcher
		int err = m_proactor->add_watcher(m_read_watcher,fd,EV_READ,this,&on_read);
		if (err != 0)
			return err;

		err = m_proactor->add_watcher(m_write_watcher,fd,EV_WRITE,this,&on_write);
		if (err != 0)
		{
			m_proactor->remove_watcher(m_read_watcher);
			return err;
		}

		m_handler = handler;

		return 0;
	}

	void AsyncSocket::close()
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		if (m_handler)
		{
			m_handler = 0;

			// Empty the queues
			for (std::deque<AsyncRead>::iterator i=m_async_reads.begin(); i!=m_async_reads.end(); ++i)
				i->m_buffer->release();

			m_async_reads.clear();

			for (std::deque<OOBase::Buffer*>::iterator i=m_async_writes.begin(); i!=m_async_writes.end(); ++i)
				(*i)->release();

			m_async_writes.clear();

			::close(m_read_watcher->fd);

			m_proactor->remove_watcher(m_read_watcher);
			m_read_watcher = 0;
			m_proactor->remove_watcher(m_write_watcher);
			m_write_watcher = 0;
		}
	}

	int AsyncSocket::read(OOBase::Buffer* buffer, size_t len)
	{
		assert(buffer);

		if (len > 0)
		{
			int err = buffer->space(len);
			if (err != 0)
				return err;
		}

		AsyncRead read = { buffer->duplicate(), len };

		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		if (!m_read_watcher)
		{
			read.m_buffer->release();
			return ENOENT;
		}

		try
		{
			m_async_reads.push_back(read);
		}
		catch (std::exception&)
		{
			read.m_buffer->release();
			return ENOMEM;
		}

		if (m_current_read.m_buffer)
			return 0;

		return read_next();
	}

	int AsyncSocket::read_next()
	{
		do
		{
			try
			{
				if (m_async_reads.empty())
					break;

				m_current_read = m_async_reads.front();
				m_async_reads.pop_front();
			}
			catch (std::exception&)
			{
				return ENOMEM;
			}

		}
		while (!do_read(m_current_read.m_to_read));

		return m_proactor->start_watcher(m_read_watcher);
	}

	bool AsyncSocket::do_read(size_t to_read)
	{
		bool inexact = false;
		if (to_read == 0)
		{
			to_read = m_current_read.m_buffer->space();
			inexact = true;
		}

		// Loop reading what we have...
		bool closed = false;
		int err = 0;
		while (to_read > 0)
		{
			ssize_t have_read = recv(m_read_watcher->fd,m_current_read.m_buffer->wr_ptr(),to_read,0);
			if (have_read == 0)
			{
				closed = true;
				break;
			}
			else if (have_read < 0)
			{
				if (errno == EAGAIN)
				{
					if (!inexact || m_current_read.m_buffer->length() == 0)
					{
						m_async_reads.push_front(m_current_read);
						return true;
					}
				}
				else
					err = errno;
				break;
			}

			// Update wr_ptr
			m_current_read.m_buffer->wr_ptr(static_cast<size_t>(have_read));

			if (m_current_read.m_to_read)
				m_current_read.m_to_read -= static_cast<size_t>(have_read);

			to_read -= static_cast<size_t>(have_read);
		}

		// Work out if we are a close or an 'actual' error
		if (err == EPIPE)
		{
			closed = true;
			err = 0;
		}

		// Call handlers
		if (m_handler && (m_current_read.m_buffer->length() || err != 0))
			m_handler->on_read(this,m_current_read.m_buffer,err);

		if (m_handler && closed)
			m_handler->on_closed(this);

		// Release the completed buffer
		m_current_read.m_buffer->release();
		m_current_read.m_buffer = 0;

		return false;
	}

	int AsyncSocket::write(OOBase::Buffer* buffer)
	{
		assert(buffer);

		if (buffer->length() == 0)
			return 0;

		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		if (!m_write_watcher)
			return ENOENT;

		try
		{
			m_async_writes.push_back(buffer->duplicate());
		}
		catch (std::exception&)
		{
			buffer->release();
			return ENOMEM;
		}

		if (m_current_write)
			return 0;

		return write_next();
	}

	int AsyncSocket::write_next()
	{
		do
		{
			try
			{
				if (m_async_writes.empty())
					return 0;

				m_current_write = m_async_writes.front();
				m_async_writes.pop_front();
			}
			catch (std::exception&)
			{
				return ENOMEM;
			}

		}
		while (!do_write());

		return m_proactor->start_watcher(m_write_watcher);
	}

	bool AsyncSocket::do_write()
	{
		// Loop writing what we can...
		size_t to_write = m_current_write->length();

		bool bSent = false;
		int err = 0;
		while (to_write > 0)
		{
			ssize_t have_sent = send(m_write_watcher->fd,m_current_write->rd_ptr(),to_write,MSG_NOSIGNAL);
			if (have_sent <= 0)
			{
				if (errno == EAGAIN)
				{
					m_async_writes.push_front(m_current_write);
					return true;
				}

				err = errno;
				break;
			}

			// Update rd_ptr
			m_current_write->rd_ptr(have_sent);

			to_write -= static_cast<size_t>(have_sent);

			bSent = true;
		}

		// Work out if we are a close or an 'actual' error
		bool closed = false;
		if (err == EPIPE)
		{
			closed = true;
			err = 0;
		}

		// Call handlers
		if (m_handler && (bSent || err != 0))
			m_handler->on_write(this,m_current_write,err);

		if (m_handler && closed)
			m_handler->on_closed(this);

		// Release the completed buffer
		m_current_write->release();
		m_current_write = 0;

		return false;
	}

	void AsyncSocket::on_read(void* param)
	{
		static_cast<AsyncSocket*>(param)->read_next();
	}

	void AsyncSocket::on_write(void* param)
	{
		static_cast<AsyncSocket*>(param)->write_next();
	}

	template <typename SocketType>
	int AcceptSocket<SocketType>::init(OOSvrBase::Acceptor* handler, int fd)
	{
		OOBase::Guard<OOBase::Condition::Mutex> guard(m_lock);

		// Add our watcher
		int err = m_proactor->add_watcher(m_watcher,fd,EV_READ,this,&on_accept);
		if (err != 0)
			return err;

		err = m_proactor->start_watcher(m_watcher);
		if (err != 0)
		{
			m_proactor->remove_watcher(m_watcher);
			return err;
		}

		m_handler = handler;

		return 0;
	}

	template <typename SocketType>
	void AcceptSocket<SocketType>::on_accept(void* param)
	{
		return static_cast<AcceptSocket*>(param)->on_accept_i();
	}

	template <typename SocketType>
	void AcceptSocket<SocketType>::on_accept_i()
	{
		OOBase::Guard<OOBase::Condition::Mutex> guard(m_lock);

		// http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod#The_special_problem_of_accept_ing_wh
		// Problem with accept...
		void* EV_TODO;

		// Call accept on the fd...
		int err = 0;
		socklen_t len = 0;
		int new_fd = accept(m_watcher->fd,NULL,&len);
		if (new_fd == -1)
		{
			err = errno;
			if (err == EAGAIN)
			{
				// libev can give spurious readiness...
				return;
			}
		}
		else
		{
			// Add FD_CLOEXEC
			err = OOBase::POSIX::fcntl_addfd(new_fd,FD_CLOEXEC);
			if (err != 0)
				::close(new_fd);
		}

		// Prepare socket if okay
		SocketType* pSocket = 0;
		if (err == 0)
		{
			OOBASE_NEW(pSocket,SocketType(new_fd,m_path));
			if (!pSocket)
			{
				err = ENOMEM;
				::close(new_fd);
			}
		}

		// Call the acceptor if we are not closing...
		bool again = false;
		if (err != 0 || !m_closing)
			again = m_handler->on_accept(pSocket,err) && (err == 0);

		if (!again)
		{
			// close socket - close the watcher...
			::close(m_watcher->fd);
			m_proactor->remove_watcher(m_watcher);
			m_watcher = 0;
		}

		// If we are closing
		if (m_closing)
			m_close_cond.signal();
	}

	template <typename SocketType>
	void AcceptSocket<SocketType>::close()
	{
		OOBase::Guard<OOBase::Condition::Mutex> guard(m_lock);

		if (!m_closing)
		{
			m_closing = true;

			if (m_watcher)
				shutdown(m_watcher->fd,SHUT_RDWR);

			// Wait for the watcher to be killed
			while (m_watcher)
				m_close_cond.wait(m_lock);
		}
	}
}

OOSvrBase::Ev::ProactorImpl::ProactorImpl() :
		m_pLoop(0), m_pIOQueue(0), m_bStop(false)
{
	// Create an ev loop
	m_pLoop = ev_loop_new(EVFLAG_AUTO | EVFLAG_NOENV);
	if (!m_pLoop)
		OOBase_CallCriticalFailure("ev_loop_new failed");

	// Init the async watcher so we can alert the loop
	ev_async_init(&m_alert,&on_alert);
	m_alert.data = this;

	ev_async_start(m_pLoop,&m_alert);

	// Get number of processors in a better way...
	void* POSIX_TODO;

	// Create the worker pool
	int num_procs = 2;
	for (int i=0; i<num_procs; ++i)
	{
		OOBase::SmartPtr<OOBase::Thread> ptrThread;
		OOBASE_NEW(ptrThread,OOBase::Thread());

		ptrThread->run(&worker,this);

		m_workers.push_back(ptrThread);
	}
}

OOSvrBase::Ev::ProactorImpl::~ProactorImpl()
{
	// Acquire the global lock
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	m_bStop = true;

	// Alert the loop
	ev_async_send(m_pLoop,&m_alert);

	guard.release();

	// Wait for all the threads to finish
	for (std::vector<OOBase::SmartPtr<OOBase::Thread> >::iterator i=m_workers.begin(); i!=m_workers.end(); ++i)
	{
		(*i)->join();
	}

	// Done with the loop
	ev_loop_destroy(m_pLoop);
}

int OOSvrBase::Ev::ProactorImpl::worker(void* param)
{
	try
	{
		return static_cast<ProactorImpl*>(param)->worker_i();
	}
	catch (...)
	{
		return EINVAL;
	}
}

int OOSvrBase::Ev::ProactorImpl::worker_i()
{
	std::deque<io_watcher*> io_queue;

	for (;;)
	{
		OOBase::Guard<OOBase::Mutex> guard(m_ev_lock);

		// Swap over the IO queue to our local one...
		m_pIOQueue = &io_queue;

		// Loop while we are just processing async's
		do
		{
			m_bAsyncTriggered = false;

			// Run the ev loop once with the lock held
			ev_loop(m_pLoop,EVLOOP_ONESHOT);

			if (m_bAsyncTriggered)
			{
				// Acquire the global lock
				OOBase::Guard<OOBase::SpinLock> guard2(m_lock);

				// Check for add and remove of watchers
				for (std::deque<io_info>::iterator i=m_update_queue.begin(); i!=m_update_queue.end(); ++i)
				{
					if (i->op == 0)
					{
						// Add to the loop
						ev_io_start(m_pLoop,i->watcher);
					}
					else
					{
						// Stop the watcher
						ev_io_stop(m_pLoop,i->watcher);

						if (i->op == 2)
						{
							// Remove from the pending queue...
							std::deque<io_watcher*>::iterator j = std::remove_if(io_queue.begin(),io_queue.end(),std::bind2nd(std::equal_to<io_watcher*>(),i->watcher));
							io_queue.erase(j,io_queue.end());

							delete i->watcher;
						}
					}
				}
				m_update_queue.clear();

				// Check for stop...
				if (m_bStop)
					return 0;
			}
		}
		while (io_queue.empty());

		// Release the loop mutex...
		guard.release();

		// Process our queue
		for (std::deque<io_watcher*>::iterator i=io_queue.begin(); i!=io_queue.end(); ++i)
		{
			if ((*i)->callback)
				(*(*i)->callback)((*i)->param);
		}
		io_queue.clear();
	}
}

int OOSvrBase::Ev::ProactorImpl::add_watcher(io_watcher*& pNew, int fd, int events, void* param, void (*callback)(void*))
{
	pNew = 0;
	OOBASE_NEW(pNew,io_watcher());
	if (!pNew)
		return ENOMEM;

	ev_io_init(pNew,&on_io,fd,events);
	pNew->data = this;
	pNew->param = param;
	pNew->callback = callback;

	return 0;
}

int OOSvrBase::Ev::ProactorImpl::remove_watcher(io_watcher* watcher)
{
	try
	{
		// Add to the queue
		OOBase::Guard<OOBase::SpinLock> guard(m_lock);

		io_info info = { watcher, 2 };
		m_update_queue.push_back(info);

		// Alert the loop
		ev_async_send(m_pLoop,&m_alert);

		return 0;
	}
	catch (std::exception&)
	{
		return ENOMEM;
	}
}

int OOSvrBase::Ev::ProactorImpl::start_watcher(io_watcher* watcher)
{
	try
	{
		// Add to the queue
		OOBase::Guard<OOBase::SpinLock> guard(m_lock);

		io_info info = { watcher, 0 };
		m_update_queue.push_back(info);

		// Alert the loop
		ev_async_send(m_pLoop,&m_alert);

		return 0;
	}
	catch (std::exception&)
	{
		return ENOMEM;
	}
}

int OOSvrBase::Ev::ProactorImpl::stop_watcher(io_watcher* watcher)
{
	try
	{
		// Add to the queue
		OOBase::Guard<OOBase::SpinLock> guard(m_lock);

		io_info info = { watcher, 1 };
		m_update_queue.push_back(info);

		// Alert the loop
		ev_async_send(m_pLoop,&m_alert);

		return 0;
	}
	catch (std::exception&)
	{
		return ENOMEM;
	}
}

void OOSvrBase::Ev::ProactorImpl::on_alert(ev_loop_t*, ev_async* w, int)
{
	static_cast<ProactorImpl*>(w->data)->m_bAsyncTriggered = true;
}

void OOSvrBase::Ev::ProactorImpl::on_io(ev_loop_t*, ev_io* w, int events)
{
	static_cast<ProactorImpl*>(w->data)->on_io_i(static_cast<io_watcher*>(w),events);
}

void OOSvrBase::Ev::ProactorImpl::on_io_i(io_watcher* watcher, int events)
{
	try
	{
		// Add ourselves to the pending io queue
		m_pIOQueue->push_back(watcher);

		// Stop the watcher - we will restart it later...
		ev_io_stop(m_pLoop,watcher);
	}
	catch (std::exception& e)
	{
		OOBase_CallCriticalFailure(e.what());
	}
}

OOBase::Socket* OOSvrBase::Ev::ProactorImpl::accept_local(Acceptor* handler, const std::string& path, int* perr, SECURITY_ATTRIBUTES* psa)
{
	// path is a UNIX pipe name - e.g. /tmp/ooserverd

	int fd = socket(PF_UNIX,SOCK_STREAM,0);
	if (fd == -1)
	{
		*perr = errno;
		return 0;
	}

	// Set non-blocking
	*perr = OOBase::POSIX::fcntl_addfl(O_NONBLOCK);
	if (*perr != 0)
	{
		close(fd);
		return 0;
	}

	// Add FD_CLOEXEC
	*perr = OOBase::POSIX::fcntl_addfd(fd,FD_CLOEXEC);
	if (*perr != 0)
	{
		close(fd);
		return 0;
	}

	// Compose filename
	sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	memset(addr.sun_path,0,sizeof(addr.sun_path));
	path.copy(addr.sun_path,sizeof(addr.sun_path)-1);

	// Unlink any existing inode
	unlink(path.c_str());

	// Bind...
	if (bind(fd,reinterpret_cast<sockaddr*>(&addr),sizeof(addr)) != 0)
	{
		*perr = errno;
		close(fd);
		return 0;
	}

	// Chmod
	mode_t mode = 0777;
	if (psa)
		mode = psa->mode;

	if (chmod(path.c_str(),mode) != 0)
	{
		*perr = errno;
		close(fd);
		return 0;
	}

	// Listen...
	if (listen(fd,0) != 0)
	{
		*perr = errno;
		close(fd);
		return 0;
	}

	// Wrap up in a controlling socket class
	OOBase::SmartPtr<AcceptSocket<OOBase::POSIX::LocalSocket> > pAccept = 0;
	OOBASE_NEW(pAccept,AcceptSocket<OOBase::POSIX::LocalSocket>(this,path));
	if (!pAccept)
		*perr = ENOMEM;
	else
		*perr = pAccept->init(handler,fd);

	if (*perr != 0)
	{
		close(fd);
		return 0;
	}

	return pAccept.detach();
}

OOSvrBase::AsyncSocket* OOSvrBase::Ev::ProactorImpl::attach_socket(IOHandler* handler, int* perr, OOBase::Socket* sock)
{
	assert(perr);
	assert(sock);

	// Cast to our known base
	OOBase::POSIX::Socket* pOrigSock = static_cast<OOBase::POSIX::Socket*>(sock);

	// Duplicate the contained handle
	int new_fd = dup(pOrigSock->get_fd());
	if (new_fd == -1)
	{
		*perr = errno;
		return 0;
	}

	// Set non-blocking
	*perr = OOBase::POSIX::fcntl_addfl(new_fd,O_NONBLOCK);
	if (*perr != 0)
	{
		close(new_fd);
		return 0;
	}

	// Add FD_CLOEXEC
	*perr = OOBase::POSIX::fcntl_addfd(new_fd,FD_CLOEXEC);
	if (*perr != 0)
	{
		close(new_fd);
		return 0;
	}

	// Alloc a new async socket
	::AsyncSocket* pSock;
	OOBASE_NEW(pSock,::AsyncSocket(this));
	if (!pSock)
	{
		close(new_fd);
		*perr = ENOMEM;
		return 0;
	}

	*perr = pSock->bind(handler,new_fd);
	if (*perr != 0)
	{
		close(new_fd);
		pSock->release();
		return 0;
	}

	return pSock;
}

#endif // HAVE_EV_H
