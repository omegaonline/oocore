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

#if !defined(STATUS_PIPE_BROKEN)
#define STATUS_PIPE_BROKEN 0xC000014BL
#endif

#include "Win32Socket.h"
#include "ProactorWin32.h"

OOSvrBase::Win32::ProactorImpl::ProactorImpl()
{
}

OOSvrBase::Win32::ProactorImpl::~ProactorImpl()
{
	// Spin while we have outstanding requests...
	OOBase::timeval_t wait(30);
	OOBase::Countdown countdown(&wait);
	while (wait != OOBase::timeval_t::Zero && m_outstanding.value() != 0)
	{
		OOBase::sleep(OOBase::timeval_t(0,50000));

		countdown.update();
	}

	// We should have halted here
	assert(m_outstanding == 0);
}

OOSvrBase::AsyncSocket* OOSvrBase::Win32::ProactorImpl::attach_socket(IOHandler* handler, int* perr, OOBase::Socket* sock)
{
	assert(perr);
	assert(sock);

	// Cast to our known base
	OOBase::Win32::Socket* pOrigSock = static_cast<OOBase::Win32::Socket*>(sock);

	// Alloc a ShmSocket
	HandleSocket* pSock;
	OOBASE_NEW(pSock,HandleSocket(this,pOrigSock->peek_handle()));
	if (!pSock)
	{
		*perr = ERROR_OUTOFMEMORY;
		return 0;
	}

	*perr = pSock->init(handler);
	if (*perr != 0)
	{
		pSock->release();
		return 0;
	}

	void* TODO; // Duplicate the incoming handle

	// Now swap out the handle
	pOrigSock->detach_handle();	

	return pSock;
}

OOSvrBase::Win32::HandleSocket::HandleSocket(OOSvrBase::Win32::ProactorImpl* pProactor, HANDLE handle) :
	m_pProactor(pProactor),
	m_handle(handle),
	m_handler(0)
{
	m_read_complete.m_is_reading = true;
	m_read_complete.m_buffer = 0;
	m_read_complete.m_this_ptr = this;
	m_write_complete.m_is_reading = false;
	m_write_complete.m_buffer = 0;
	m_write_complete.m_this_ptr = this;
}

OOSvrBase::Win32::HandleSocket::~HandleSocket()
{
	close();
}

int OOSvrBase::Win32::HandleSocket::init(OOSvrBase::IOHandler* handler)
{
	if (!OOBase::Win32::BindIoCompletionCallback(m_handle,&completion_fn,0))
		return GetLastError();

	m_handler = handler;

	return 0;
}

int OOSvrBase::Win32::HandleSocket::read(OOBase::Buffer* buffer, size_t len)
{
	assert(buffer);

	if (len > 0)
	{
		int err = buffer->space(len);
		if (err != 0)
			return err;
	}

	AsyncRead read = { buffer->duplicate(), len };

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	try
	{
		m_async_reads.push_back(read);	
	}
	catch (std::exception&)
	{
		read.m_buffer->release();
		return ERROR_OUTOFMEMORY;
	}
	
	if (m_read_complete.m_buffer)
		return 0;
	
	return read_next();
}

int OOSvrBase::Win32::HandleSocket::read_next()
{
	do
	{
		AsyncRead read;
		try
		{
			if (m_async_reads.empty())
				return 0;

			read = m_async_reads.front();
			m_async_reads.pop_front();
		}
		catch (std::exception&)
		{
			return ERROR_OUTOFMEMORY;
		}

		// Reset the completion info
		memset(&m_read_complete.m_ov,0,sizeof(OVERLAPPED));
		m_read_complete.m_buffer = read.m_buffer;
		m_read_complete.m_to_read = read.m_to_read;
	} while (!do_read(static_cast<DWORD>(m_read_complete.m_to_read)));

	return 0;
}

int OOSvrBase::Win32::HandleSocket::write(OOBase::Buffer* buffer)
{
	assert(buffer);

	if (buffer->length() == 0)
		return 0;

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	try
	{
		m_async_writes.push_back(buffer->duplicate());	
	}
	catch (std::exception&)
	{
		buffer->release();
		return ERROR_OUTOFMEMORY;
	}
	
	if (m_write_complete.m_buffer)
		return 0;
	
	return write_next();
}

int OOSvrBase::Win32::HandleSocket::write_next()
{
	do
	{
		OOBase::Buffer* buffer = 0;
		try
		{
			if (m_async_writes.empty())
				return 0;

			buffer = m_async_writes.front();
			m_async_writes.pop_front();
		}
		catch (std::exception&)
		{
			return ERROR_OUTOFMEMORY;
		}

		// Reset the completion info
		memset(&m_write_complete.m_ov,0,sizeof(OVERLAPPED));
		m_write_complete.m_buffer = buffer;
			
	} while (!do_write());
	
	return 0;
}

VOID CALLBACK OOSvrBase::Win32::HandleSocket::completion_fn(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	Completion* pInfo = CONTAINING_RECORD(lpOverlapped,Completion,m_ov);
	HandleSocket* this_ptr = pInfo->m_this_ptr;
	if (pInfo->m_is_reading)
		this_ptr->handle_read(dwErrorCode,dwNumberOfBytesTransfered);
	else
		this_ptr->handle_write(dwErrorCode,dwNumberOfBytesTransfered);
	
	--this_ptr->m_pProactor->m_outstanding;
	this_ptr->release();
}

bool OOSvrBase::Win32::HandleSocket::do_read(DWORD dwToRead)
{
	addref();
	++m_pProactor->m_outstanding;

	if (dwToRead == 0)
		dwToRead = static_cast<DWORD>(m_read_complete.m_buffer->space());

	DWORD dwRead = 0;
	if (!ReadFile(m_handle,m_read_complete.m_buffer->wr_ptr(),dwToRead,&dwRead,&m_read_complete.m_ov))
	{
		DWORD dwErr = GetLastError();
		if (dwErr != ERROR_IO_PENDING)
		{
			// Update wr_ptr
			m_read_complete.m_buffer->wr_ptr(dwRead);

			bool closed = false;
			if (dwErr == ERROR_BROKEN_PIPE)
				closed = true;

			if (closed)
				dwErr = 0;

			if (m_handler && m_read_complete.m_buffer->length())
				m_handler->on_read(this,m_read_complete.m_buffer,dwErr);

			if (m_handler && closed)
				m_handler->on_closed(this);
			
			m_read_complete.m_buffer->release();
			m_read_complete.m_buffer = 0;

			--m_pProactor->m_outstanding;
			release();

			return closed;
		}
	}

	return true;
}

void OOSvrBase::Win32::HandleSocket::handle_read(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Update wr_ptr
	m_read_complete.m_buffer->wr_ptr(dwNumberOfBytesTransfered);

	if (m_read_complete.m_to_read)
		m_read_complete.m_to_read -= dwNumberOfBytesTransfered;

	bool closed = false;
	if (dwErrorCode == 0 && m_read_complete.m_to_read > 0)
	{
		// More to read
		if (do_read(static_cast<DWORD>(m_read_complete.m_to_read)))
			return;
	}
	else
	{
		// Work out if we are a close or an 'actual' error
		if (dwErrorCode == STATUS_PIPE_BROKEN)
			closed = true;

		if (closed)
			dwErrorCode = 0;

		// Call handlers 
		if (m_handler && m_read_complete.m_buffer->length())
			m_handler->on_read(this,m_read_complete.m_buffer,dwErrorCode);

		if (m_handler && closed)
			m_handler->on_closed(this);

		// Release the completed buffer
		m_read_complete.m_buffer->release();
		m_read_complete.m_buffer = 0;
	}

	// Read the next
	if (!closed)
		read_next();
}

bool OOSvrBase::Win32::HandleSocket::do_write()
{
	addref();
	++m_pProactor->m_outstanding;

	DWORD dwWritten = 0;
	if (!WriteFile(m_handle,m_write_complete.m_buffer->rd_ptr(),static_cast<DWORD>(m_write_complete.m_buffer->length()),&dwWritten,&m_write_complete.m_ov))
	{
		DWORD dwErr = GetLastError();
		if (dwErr != ERROR_IO_PENDING)
		{
			// Update rd_ptr
			m_write_complete.m_buffer->rd_ptr(dwWritten);

			bool closed = false;
			if (dwErr == ERROR_BROKEN_PIPE)
				closed = true;

			if (closed)
				dwErr = 0;

			if (m_handler && m_write_complete.m_buffer->length())
				m_handler->on_write(this,m_write_complete.m_buffer,dwErr);

			if (m_handler && closed)
				m_handler->on_closed(this);
			
			m_write_complete.m_buffer->release();
			m_write_complete.m_buffer = 0;

			--m_pProactor->m_outstanding;
			release();

			return closed;
		}
	}

	return true;
}

void OOSvrBase::Win32::HandleSocket::handle_write(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// Update rd_ptr
	m_write_complete.m_buffer->rd_ptr(dwNumberOfBytesTransfered);
	
	bool closed = false;
	if (dwErrorCode == 0 && m_write_complete.m_buffer->length() > 0)
	{
		// More to send
		if (do_write())
			return;
	}
	else
	{
		// Work out if we are a close or an 'actual' error
		if (dwErrorCode == STATUS_PIPE_BROKEN)
			closed = true;

		if (closed)
			dwErrorCode = 0;

		// Call handlers 
		if (m_handler && m_write_complete.m_buffer->length())
			m_handler->on_write(this,m_write_complete.m_buffer,dwErrorCode);

		if (m_handler && closed)
			m_handler->on_closed(this);

		// Release the completed buffer
		m_write_complete.m_buffer->release();
		m_write_complete.m_buffer = 0;
	}

	// Write the next
	if (!closed)
		write_next();	
}

void OOSvrBase::Win32::HandleSocket::close()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	m_handler = 0;

	do
	{
	} while ((void)0,false);

	// Empty the queues
	for (std::deque<AsyncRead>::iterator i=m_async_reads.begin();i!=m_async_reads.end();++i)
		i->m_buffer->release();

	m_async_reads.clear();

	for (std::deque<OOBase::Buffer*>::iterator i=m_async_writes.begin();i!=m_async_writes.end();++i)
		(*i)->release();

	m_async_writes.clear();

	if (m_handle.is_valid())
	{
		CancelIo(m_handle);
		CloseHandle(m_handle.detach());
	}
}

#endif // _WIN32
