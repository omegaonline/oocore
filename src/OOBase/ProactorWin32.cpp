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

OOSvrBase::ProactorImpl::ProactorImpl()
{
}

OOSvrBase::ProactorImpl::~ProactorImpl()
{
}

OOSvrBase::HandleSocket::HandleSocket(OOSvrBase::ProactorImpl* pProactor, HANDLE handle) :
	m_pProactor(pProactor),
	m_handle(handle),
	m_handler(0)
{
}

OOSvrBase::HandleSocket::~HandleSocket()
{
	close();
}

int OOSvrBase::HandleSocket::init(OOSvrBase::IOHandler* handler)
{
	if (!OOBase::Win32::BindIoCompletionCallback(m_handle,&completion_fn,0))
		return GetLastError();

	m_handler = handler;

	return 0;
}

int OOSvrBase::HandleSocket::read(OOBase::Buffer* buffer, size_t len)
{
	assert(buffer);

	if (len > 0)
	{
		int err = buffer->space(len);
		if (err != 0)
			return err;
	}
	
	Completion* pInfo = 0;
	OOBASE_NEW(pInfo,Completion);
	if (!pInfo)
		return ERROR_OUTOFMEMORY;

	ZeroMemory(&pInfo->ov,sizeof(OVERLAPPED));
	pInfo->buffer = buffer->duplicate();
	pInfo->this_ptr = this;
	pInfo->is_reading = true;
	pInfo->to_read = len;
	
	int err = do_read(pInfo,static_cast<DWORD>(len));
	if (err != 0)
	{
		pInfo->buffer->release();
		delete pInfo;
	}
	return err;
}

int OOSvrBase::HandleSocket::write(OOBase::Buffer* buffer)
{
	assert(buffer);

	if (buffer->length() == 0)
		return 0;

	Completion* pInfo = 0;
	OOBASE_NEW(pInfo,Completion);
	if (!pInfo)
		return ERROR_OUTOFMEMORY;

	ZeroMemory(&pInfo->ov,sizeof(OVERLAPPED));
	pInfo->buffer = buffer->duplicate();
	pInfo->this_ptr = this;
	pInfo->is_reading = false;
	
	int err = do_write(pInfo);
	if (err != 0)
	{
		pInfo->buffer->release();
		delete pInfo;
	}
	return err;
}

VOID CALLBACK OOSvrBase::HandleSocket::completion_fn(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	Completion* pInfo = CONTAINING_RECORD(lpOverlapped,Completion,ov);
	HandleSocket* this_ptr = pInfo->this_ptr;
	if (pInfo->is_reading)
		this_ptr->handle_read(dwErrorCode,dwNumberOfBytesTransfered,pInfo);
	else
		this_ptr->handle_write(dwErrorCode,dwNumberOfBytesTransfered,pInfo);

	--this_ptr->m_pProactor->m_outstanding;
	this_ptr->release();
}

DWORD OOSvrBase::HandleSocket::do_read(Completion* pInfo, DWORD dwToRead)
{
	addref();
	++m_pProactor->m_outstanding;

	if (dwToRead == 0)
		dwToRead = static_cast<DWORD>(pInfo->buffer->space());
	
	if (!ReadFile(m_handle,pInfo->buffer->wr_ptr(),dwToRead,NULL,&pInfo->ov))
	{
		DWORD ret = GetLastError();
		if (ret != ERROR_IO_PENDING)
		{
			--m_pProactor->m_outstanding;
			release();
			return ret;
		}
	}

	return 0;
}

void OOSvrBase::HandleSocket::handle_read(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, Completion* pInfo)
{
	// Update rd_ptr
	pInfo->buffer->wr_ptr(dwNumberOfBytesTransfered);

	if (pInfo->to_read)
		pInfo->to_read -= dwNumberOfBytesTransfered;

	bool more = (pInfo->to_read > 0);
	
	if (dwErrorCode == 0 && more)
	{
		// More to read
		dwErrorCode = do_read(pInfo,static_cast<DWORD>(pInfo->to_read));
	}

	bool closed = false;
	if (dwErrorCode == STATUS_PIPE_BROKEN)
		closed = true;

	if (dwErrorCode != 0 || !more)
	{
		// Call handler
		if (!closed || pInfo->buffer->length())
			m_handler->on_read(this,pInfo->buffer,dwErrorCode);

		if (closed)
			m_handler->on_closed(this);

		pInfo->buffer->release();
		delete pInfo;
	}
}

DWORD OOSvrBase::HandleSocket::do_write(Completion* pInfo)
{
	addref();
	++m_pProactor->m_outstanding;

	if (!WriteFile(m_handle,pInfo->buffer->rd_ptr(),static_cast<DWORD>(pInfo->buffer->length()),NULL,&pInfo->ov))
	{
		DWORD ret = GetLastError();
		if (ret != ERROR_IO_PENDING)
		{
			--m_pProactor->m_outstanding;
			release();
			return ret;
		}
	}

	return 0;
}

void OOSvrBase::HandleSocket::handle_write(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, Completion* pInfo)
{
	// Update rd_ptr
	pInfo->buffer->rd_ptr(dwNumberOfBytesTransfered);
	bool more = (pInfo->buffer->length() > 0);

	if (dwErrorCode == 0 && more)
	{
		// More to send
		dwErrorCode = do_write(pInfo);
	}

	bool closed = false;
	if (dwErrorCode == STATUS_PIPE_BROKEN)
		closed = true;

	if (dwErrorCode != 0 || !more)
	{
		// Call handler
		if (!closed || pInfo->buffer->length())
			m_handler->on_write(this,pInfo->buffer,dwErrorCode);

		if (closed)
			m_handler->on_closed(this);

		pInfo->buffer->release();
		delete pInfo;
	}
}

void OOSvrBase::HandleSocket::close()
{
	if (m_handle && m_handle != INVALID_HANDLE_VALUE)
	{
		CancelIo(m_handle);
		CloseHandle(m_handle.detach());
	}
}

#endif // _WIN32
