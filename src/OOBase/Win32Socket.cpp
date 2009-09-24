///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOBase, the Omega Online Base library.
//
// OOBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "Win32Socket.h"

#if defined(_WIN32)

OOBase::Win32::SocketImpl::SocketImpl(HANDLE hSocket) :
	m_hSocket(hSocket),
	m_hReadEvent(NULL),
	m_hWriteEvent(NULL)
{
}

OOBase::Win32::SocketImpl::~SocketImpl()
{
	close();

	if (m_hReadEvent)
		CloseHandle(m_hReadEvent);

	if (m_hWriteEvent)
		CloseHandle(m_hWriteEvent);
}

int OOBase::Win32::SocketImpl::send(const void* buf, size_t len, const OOBase::timeval_t* timeout)
{
	if (!m_hWriteEvent)
	{
		m_hWriteEvent = CreateEventW(NULL,TRUE,FALSE,NULL);
		if (!m_hWriteEvent)
			return GetLastError();
	}

	OVERLAPPED ov = {0};
	ov.hEvent = m_hWriteEvent;
	
	const char* cbuf = reinterpret_cast<const char*>(buf);
	while (len > 0)
	{
		DWORD dwWritten = 0;
		if (WriteFile(m_hSocket,cbuf,static_cast<DWORD>(len),&dwWritten,&ov))
		{
			if (!SetEvent(m_hWriteEvent))
				return GetLastError();
		}
		else
		{
			DWORD ret = GetLastError();
			if (ret != ERROR_IO_PENDING)
				return ret;

			// Wait...
			if (timeout)
			{
				DWORD dwWait = WaitForSingleObject(ov.hEvent,timeout->msec());
				if (dwWait == WAIT_TIMEOUT)
				{
					CancelIo(m_hSocket);
					return WAIT_TIMEOUT;
				}
				else if (dwWait != WAIT_OBJECT_0)
				{
					ret = GetLastError();
					CancelIo(m_hSocket);
					return ret;
				}
			}
		}
			
		if (!GetOverlappedResult(m_hSocket,&ov,&dwWritten,TRUE))
		{
			DWORD ret = GetLastError();
			CancelIo(m_hSocket);
			return ret;
		}
		
		cbuf += dwWritten;
		len -= dwWritten;
	}

	return 0;
}

size_t OOBase::Win32::SocketImpl::recv(void* buf, size_t len, int* perr, const OOBase::timeval_t* timeout)
{
	assert(perr);
	*perr = 0;

	if (!m_hReadEvent)
	{
		m_hReadEvent = CreateEventW(NULL,TRUE,FALSE,NULL);
		if (!m_hReadEvent)
		{
			*perr = GetLastError();
			return 0;
		}
	}

	OVERLAPPED ov = {0};
	ov.hEvent = m_hReadEvent;
	
	char* cbuf = reinterpret_cast<char*>(buf);
	size_t total = len;
	while (total > 0)
	{
		DWORD dwRead = 0;
		if (ReadFile(m_hSocket,cbuf,static_cast<DWORD>(total),&dwRead,&ov))
		{
			if (!SetEvent(m_hReadEvent))
			{
				*perr = GetLastError();
				return 0;
			}
		}
		else
		{
			*perr = GetLastError();
			if (*perr == ERROR_MORE_DATA)
				dwRead = static_cast<DWORD>(total);
			else
			{
				if (*perr != ERROR_IO_PENDING)
					return (len - total);

				// Wait...
				if (timeout)
				{
					DWORD dwWait = WaitForSingleObject(ov.hEvent,timeout->msec());
					if (dwWait == WAIT_TIMEOUT)
					{
						CancelIo(m_hSocket);
						*perr = WAIT_TIMEOUT;
						return (len - total);
					}
					else if (dwWait != WAIT_OBJECT_0)
					{
						*perr = GetLastError();
						CancelIo(m_hSocket);
						return (len - total);
					}
				}
			}
		}

		if (!GetOverlappedResult(m_hSocket,&ov,&dwRead,TRUE))
		{
			*perr = GetLastError();
			CancelIo(m_hSocket);
			return (len - total);
		}

		cbuf += dwRead;
		total -= dwRead;
	}

	*perr = 0;
	return len;
}

void OOBase::Win32::SocketImpl::close()
{
	if (m_hSocket.is_valid())
		CloseHandle(m_hSocket.detach());
}

OOBase::LocalSocket::uid_t OOBase::Win32::LocalSocket::get_uid()
{
	if (!ImpersonateNamedPipeClient(m_hSocket))
		OOBase_CallCriticalFailure(GetLastError());

	OOBase::Win32::SmartHandle uid;
	BOOL bRes = OpenThreadToken(GetCurrentThread(),TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,FALSE,&uid);
	DWORD err = 0;
	if (!bRes)
		err = GetLastError();

	if (!RevertToSelf())
		OOBase_CallCriticalFailure(GetLastError());
	
	if (!bRes)
		OOBase_CallCriticalFailure(err);
	
	return uid.detach();
}

OOBase::LocalSocket* OOBase::LocalSocket::connect_local(const std::string& path, int* perr, const timeval_t* wait)
{
	assert(perr);
	*perr = 0;

	std::string pipe_name = "\\\\.\\pipe\\" + path;

	Win32::SmartHandle hPipe;
	for (;;)
	{
		hPipe = CreateFileA(pipe_name.c_str(),
			PIPE_ACCESS_DUPLEX,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			NULL);

		if (hPipe != INVALID_HANDLE_VALUE)
			break;
		
		DWORD dwErr = GetLastError();
		if (dwErr != ERROR_PIPE_BUSY) 
		{
			*perr = dwErr;
			return 0;
		}

		DWORD dwWait = NMPWAIT_USE_DEFAULT_WAIT;
		if (wait)
			dwWait = wait->msec();

		if (!WaitNamedPipeA(pipe_name.c_str(),dwWait))
		{
			*perr = GetLastError();
			return 0;
		}
	}
	
	LocalSocket* pSocket = 0;
	OOBASE_NEW(pSocket,Win32::LocalSocket(hPipe));
	if (!pSocket)
	{
		*perr = ERROR_OUTOFMEMORY;
		return 0;
	}
	
	hPipe.detach();
	
	return pSocket;
}

#endif // _WIN32
