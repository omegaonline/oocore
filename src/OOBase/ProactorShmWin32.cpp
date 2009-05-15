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

#if defined(_WIN32)

#include "Queue.h"
#include "Logger.h"
#include "ProactorWin32.h"
#include "Win32ShmSocket.h"

#include <list>

#if !defined(ERROR_UNIDENTIFIED_ERROR)
#define ERROR_UNIDENTIFIED_ERROR 1287L
#endif

namespace
{
	class ShmSocket : 
		public OOSvrBase::AsyncSocket,
		public OOBase::Win32::ShmSocketImpl
	{
	public:
		ShmSocket(OOSvrBase::IOHandler* handler, OOSvrBase::ProactorImpl* proactor);
		virtual ~ShmSocket();

		int init();
		virtual int read(OOBase::Buffer* buffer, size_t len = 0);
		virtual int write(OOBase::Buffer* buffer);
		virtual void close();

	private:
		OOSvrBase::IOHandler*    m_handler;
		OOSvrBase::ProactorImpl* m_proactor;

		struct IOBuf
		{
			OOBase::Buffer* buf;
			size_t          len;
		};
		
		OOBase::SpinLock m_read_lock;
		OOBase::SpinLock m_write_lock;
		HANDLE           m_hCloseWait;
		HANDLE           m_hReadWait;
		std::list<IOBuf> m_read_list;
		HANDLE           m_hWriteWait;
		std::list<IOBuf> m_write_list;

		static VOID WINAPI on_close(LPVOID lpThreadParameter, BOOLEAN TimerOrWaitFired);
		void on_close_i();

		static VOID WINAPI on_read(LPVOID lpThreadParameter, BOOLEAN TimerOrWaitFired);
		void on_read_i();

		static VOID WINAPI on_write(LPVOID lpThreadParameter, BOOLEAN TimerOrWaitFired);
		void on_write_i();
	};
}

ShmSocket::ShmSocket(OOSvrBase::IOHandler* handler, OOSvrBase::ProactorImpl* proactor) :
	m_handler(handler),
	m_proactor(proactor),
	m_hCloseWait(0),
	m_hReadWait(0),
	m_hWriteWait(0)
{
	assert(handler);
	assert(proactor);
}

ShmSocket::~ShmSocket()
{
	close();
}

int ShmSocket::init()
{
	// Inc our ref count
	addref();
	++m_proactor->m_outstanding;

	if (!RegisterWaitForSingleObject(&m_hCloseWait,m_close_event,&on_close,this,INFINITE,WT_EXECUTELONGFUNCTION | WT_EXECUTEONLYONCE))
	{
		DWORD err = GetLastError();
		--m_proactor->m_outstanding;
		release();
		return err;
	}

	return 0;
}

VOID WINAPI ShmSocket::on_close(LPVOID lpThreadParameter, BOOLEAN /*TimerOrWaitFired*/)
{
	static_cast<ShmSocket*>(lpThreadParameter)->on_close_i();
}

void ShmSocket::on_close_i()
{
	// Pipe has closed...
	OOBase::Guard<OOBase::SpinLock> guard(m_read_lock);

	UnregisterWaitEx(m_hCloseWait,NULL);
	m_hCloseWait = 0;

	// Signal our events to unblock our end...
	SetEvent(m_fifos[m_bServer ? 1 : 0].m_read_event);
	SetEvent(m_fifos[m_bServer ? 0 : 1].m_write_event);

	guard.release();
	
	m_handler->on_closed(this);

	// Done with our ref...
	--m_proactor->m_outstanding;
	release();
}

int ShmSocket::read(OOBase::Buffer* buffer, size_t len)
{
	assert(buffer);

	OOBase::Guard<OOBase::SpinLock> guard(m_read_lock);

	OOBase::Win32::ShmSocketImpl::Fifo& fifo = m_fifos[m_bServer ? 1 : 0];

	// See if we can do it now...
	if (!m_hReadWait)
	{
		// See if the fifo is empty...
		size_t total = 0;
		char* data = buffer->wr_ptr();
		size_t to_read = (len ? len : buffer->space());
		int err = 0;

		while (to_read && !fifo.is_empty())
		{
			// Recv as much as we can..
			total += fifo.recv_i(data,to_read,&err);
			if (err != 0)
				break;
		}

		// Update wr_ptr
		buffer->wr_ptr(total);

		// Remove what we have already read...
		if (len)
			len -= total;

		if (!m_hCloseWait)
			return ERROR_BROKEN_PIPE;
		
		if ((len == 0 && total != 0) || err != 0)
		{
			// Notify the handler
			guard.release();

			m_handler->on_read(this,buffer,err);
			return err;
		}
	}

	// Queue up an async read
	IOBuf rd;
	rd.buf = buffer->duplicate();
	rd.len = len;

	try
	{
		m_read_list.push_back(rd);
	}
	catch (std::exception& e)
	{
		LOG_ERROR(("std::exception thrown: %s",e.what()));
		
		rd.buf->release();
		int err = ERROR_UNIDENTIFIED_ERROR;
		guard.release();

		m_handler->on_read(this,buffer,err);
		return err;
	}

	// Set of an async wait if we don't have one outstanding
	if (!m_hReadWait)
	{
		// Inc our ref count
		addref();
		++m_proactor->m_outstanding;

		if (!RegisterWaitForSingleObject(&m_hReadWait,fifo.m_read_event,&on_read,this,INFINITE,WT_EXECUTELONGFUNCTION))
		{
			DWORD err = GetLastError();
			guard.release();

			m_handler->on_read(this,buffer,err);
			--m_proactor->m_outstanding;
			release();
			return err;
		}
	}

	return 0;
}

VOID WINAPI ShmSocket::on_read(LPVOID lpThreadParameter, BOOLEAN /*TimerOrWaitFired*/)
{
	static_cast<ShmSocket*>(lpThreadParameter)->on_read_i();
}

void ShmSocket::on_read_i()
{
	OOBase::Win32::ShmSocketImpl::Fifo& fifo = m_fifos[m_bServer ? 1 : 0];

	for (;;)
	{
		try
		{
			// Release and acquire lock each time so we allow reads to add to queue
			OOBase::Guard<OOBase::SpinLock> guard(m_read_lock);

			if (m_read_list.empty())
			{
				// Nothing more to do... stop waiting
				UnregisterWaitEx(m_hReadWait,NULL);
				m_hReadWait = 0;

				guard.release();
		
				// Release ourselves as we have no more outstanding asyncs
				--m_proactor->m_outstanding;
				release();
				return;
			}

			IOBuf& rd = m_read_list.front();

			size_t total = 0;
			char* data = rd.buf->wr_ptr();
			size_t len = (rd.len ? rd.len : rd.buf->space());
			int err = 0;
			
			// Loop while we can still read
			while (len && !fifo.is_empty())
			{
				// Recv as much as we can..
				total += fifo.recv_i(data,len,&err);

				if (err != 0)
					break;
			}

			// Update wr_ptr
			rd.buf->wr_ptr(total);

			// Remove what we have already read...
			if (rd.len)
				rd.len -= total;

			// If we have no error and more to read, return here...
			if (!err && (!total || rd.len) && m_hCloseWait != 0)
				return;

			// Pop the head
			IOBuf head = rd;
			m_read_list.pop_front();

			if (err || total)
			{
				// Release the lock before calling handlers
				guard.release();

				// Notify the handler
				m_handler->on_read(this,head.buf,err);
			}

			// Release the buffer
			head.buf->release();
		}
		catch (std::exception& e)
		{
			LOG_ERROR(("std::exception thrown: %s",e.what()));

			OOBase::Guard<OOBase::SpinLock> guard(m_read_lock);

			UnregisterWaitEx(m_hReadWait,NULL);
			m_hReadWait = 0;

			guard.release();

			// Release ourselves as we have no more outstanding asyncs
			--m_proactor->m_outstanding;
			release();
			return;
		}
	}
}

int ShmSocket::write(OOBase::Buffer* buffer)
{
	assert(buffer);

	OOBase::Guard<OOBase::SpinLock> guard(m_read_lock);

	OOBase::Win32::ShmSocketImpl::Fifo& fifo = m_fifos[m_bServer ? 0 : 1];

	if (!m_hCloseWait)
		return ERROR_BROKEN_PIPE;
	
	// See if we can do it now...
	if (!m_hWriteWait)
	{
		const char* data = buffer->rd_ptr();
		size_t len = buffer->length();
		int err = 0;

		// While we have stuff to write and the buffer isn't full
		while (len && !fifo.is_full())
		{
			// Do the send
			err = fifo.send_i(data,len);		
			if (err != 0)
				break;
		}

		// Update rd_ptr
		buffer->rd_ptr(buffer->length() - len);

		// Notify the handler if we have completed
		if (len == 0 || err != 0)
		{
			m_handler->on_write(this,buffer,err);
			return err;
		}
	}

	// Queue up an async write
	IOBuf wr;
	wr.buf = buffer->duplicate();
	wr.len = 0;

	try
	{
		m_write_list.push_back(wr);
	}
	catch (std::exception& e)
	{
		LOG_ERROR(("std::exception thrown: %s",e.what()));
		
		wr.buf->release();
		int err = ERROR_UNIDENTIFIED_ERROR;
		m_handler->on_write(this,buffer,err);
		return err;
	}

	// Set of an async wait if we don't have one outstanding
	if (!m_hWriteWait)
	{
		// Inc our ref count
		addref();
		++m_proactor->m_outstanding;

		if (!RegisterWaitForSingleObject(&m_hWriteWait,fifo.m_write_event,&on_write,this,INFINITE,WT_EXECUTEONLYONCE | WT_EXECUTELONGFUNCTION))
		{
			DWORD err = GetLastError();

			guard.release();

			m_handler->on_write(this,buffer,err);
			--m_proactor->m_outstanding;
			release();
			return err;
		}
	}

	return 0;
}

VOID WINAPI ShmSocket::on_write(LPVOID lpThreadParameter, BOOLEAN /*TimerOrWaitFired*/)
{
	static_cast<ShmSocket*>(lpThreadParameter)->on_write_i();
}

void ShmSocket::on_write_i()
{
	OOBase::Win32::ShmSocketImpl::Fifo& fifo = m_fifos[m_bServer ? 0 : 1];

	for (;;)
	{
		try
		{
			// Release and acquire lock each time so we allow writes to add to queue
			OOBase::Guard<OOBase::SpinLock> guard(m_write_lock);

			if (m_write_list.empty())
			{
				// Nothing more to do... stop waiting
				UnregisterWaitEx(m_hWriteWait,NULL);
				m_hWriteWait = 0;

				guard.release();

				// Release ourselves as we have no more outstanding asyncs
				--m_proactor->m_outstanding;
				release();
				return;
			}

			IOBuf& wr = m_write_list.front();
			int err = 0;

			if (m_hCloseWait != 0)
			{
				const char* data = wr.buf->rd_ptr();
				size_t len = wr.buf->length();
				
				// While we have stuff to write and the buffer isn't full
				while (len && !fifo.is_full())
				{
					// Do the send
					err = fifo.send_i(data,len);		
					if (err != 0)
						break;
				}

				// Update rd_ptr
				wr.buf->rd_ptr(wr.buf->length() - len);

				// If we have no error and more to write, return here...
				if (!err && len)
					return;
			}

			// Pop the head
			IOBuf head = wr;
			m_write_list.pop_front();

			if (m_hCloseWait != 0)
			{
				// Release the lock before calling handlers
				guard.release();

				// Notify the handler
				m_handler->on_write(this,head.buf,err);
			}

			// Release the buffer
			head.buf->release();
		}
		catch (std::exception& e)
		{
			LOG_ERROR(("std::exception thrown: %s",e.what()));

			OOBase::Guard<OOBase::SpinLock> guard(m_write_lock);

			UnregisterWaitEx(m_hWriteWait,NULL);
			m_hWriteWait = 0;

			guard.release();

			// Release ourselves as we have no more outstanding asyncs
			--m_proactor->m_outstanding;
			release();
			return;
		}
	}
}

void ShmSocket::close()
{
	// Acquire both locks...
	OOBase::Guard<OOBase::SpinLock> guard1(m_read_lock);
	OOBase::Guard<OOBase::SpinLock> guard2(m_write_lock);

	OOBase::Win32::ShmSocketImpl::close();
}

OOSvrBase::AsyncSocket* OOSvrBase::ProactorImpl::accept_shared_mem_socket(const std::string& strName, IOHandler* handler, int* perr, OOBase::LocalSocket* via, OOBase::timeval_t* timeout, SECURITY_ATTRIBUTES* psa)
{
	assert(perr);

	// Alloc a ShmSocket
	ShmSocket* pSock;
	OOBASE_NEW(pSock,ShmSocket(handler,this));
	if (!pSock)
	{
		*perr = ERROR_OUTOFMEMORY;
		return 0;
	}

	if (!pSock->init_server(strName,via,perr,timeout,psa))
	{
		pSock->release();
		return 0;
	}

	*perr = pSock->init();
	if (*perr != 0)
	{
		pSock->release();
		return 0;
	}
	
	return pSock;
}

OOSvrBase::AsyncSocket* OOSvrBase::ProactorImpl::connect_shared_mem_socket(IOHandler* handler, int* perr, OOBase::LocalSocket* via, OOBase::timeval_t* timeout)
{
	assert(perr);

	// Alloc a ShmSocket
	ShmSocket* pSock;
	OOBASE_NEW(pSock,ShmSocket(handler,this));
	if (!pSock)
	{
		*perr = ERROR_OUTOFMEMORY;
		return 0;
	}

	if (!pSock->init_client(via,perr,timeout))
	{
		pSock->release();
		return 0;
	}

	*perr = pSock->init();
	if (*perr != 0)
	{
		pSock->release();
		return 0;
	}
	
	return pSock;
}

#endif // _WIN32
