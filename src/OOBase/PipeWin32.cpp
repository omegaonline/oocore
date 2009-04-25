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
#include "SmartPtr.h"

#if defined(_WIN32)

#include "ProactorWin32.h"
#include "Win32Socket.h"

namespace
{
	class PipeAcceptor : public OOBase::Socket
	{
	public:
		PipeAcceptor(OOSvrBase::ProactorImpl* pProactor, const std::string& pipe_name, LPSECURITY_ATTRIBUTES psa);
		int init(OOSvrBase::Acceptor* sync_handler);

		virtual ~PipeAcceptor();
		
		virtual int send(const void* /*buf*/, size_t /*len*/, const OOBase::timeval_t* /*timeout*/ = 0)
		{
			return ERROR_INVALID_FUNCTION;
		}

		virtual size_t recv(void* /*buf*/, size_t /*len*/, int* perr, const OOBase::timeval_t* /*timeout*/ = 0)
		{
			*perr = ERROR_INVALID_FUNCTION;
			return 0;
		}

		virtual void close();

		int accept_named_pipe();

	private:
		OOSvrBase::ProactorImpl*   m_pProactor;
		OOBase::SpinLock           m_lock;
		OVERLAPPED                 m_ov;
		bool                       m_closed;
		OOSvrBase::Acceptor*       m_sync_handler;
		OOBase::Win32::SmartHandle m_hPipe;
		OOBase::Win32::SmartHandle m_hClosed;
		std::string                m_pipe_name;
		LPSECURITY_ATTRIBUTES      m_psa;
		
		static VOID CALLBACK accept_named_pipe2(PVOID lpParameter, BOOLEAN /*TimerOrWaitFired*/);
		void do_accept();
	};
}

PipeAcceptor::PipeAcceptor(OOSvrBase::ProactorImpl* pProactor, const std::string& pipe_name, LPSECURITY_ATTRIBUTES psa) :
	m_pProactor(pProactor),
	m_closed(false),
	m_sync_handler(0),
	m_hPipe(INVALID_HANDLE_VALUE),
	m_hClosed(NULL),
	m_pipe_name(pipe_name),
	m_psa(psa)
{
	ZeroMemory(&m_ov,sizeof(OVERLAPPED));
}

PipeAcceptor::~PipeAcceptor()
{
	close();

	if (m_ov.hEvent)
		CloseHandle(m_ov.hEvent);
}

int PipeAcceptor::init(OOSvrBase::Acceptor* sync_handler)
{
	m_sync_handler = sync_handler;
	
	assert(!m_ov.hEvent);
	m_ov.hEvent = CreateEventW(NULL,TRUE,TRUE,NULL);
	if (!m_ov.hEvent)
		return GetLastError();

	m_hClosed = CreateEventW(NULL,TRUE,FALSE,NULL);
	if (!m_hClosed)
	{
		DWORD res = GetLastError();
		CloseHandle(m_ov.hEvent);
		m_ov.hEvent = NULL;
		return res;
	}
		
	return 0;
}

void PipeAcceptor::close()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	m_closed = true;

	if (m_ov.hEvent)
		SetEvent(m_ov.hEvent);

	bool wait = (m_hPipe != INVALID_HANDLE_VALUE);

	guard.release();

	if (wait)
		WaitForSingleObject(m_hClosed,INFINITE);
}

int PipeAcceptor::accept_named_pipe()
{
	m_hPipe = CreateNamedPipeA(m_pipe_name.c_str(),
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_BYTE |
		PIPE_READMODE_BYTE |
		PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		0,
		0,
		0,
		m_psa);

	if (m_hPipe == INVALID_HANDLE_VALUE)
		return GetLastError();
	
	DWORD dwErr = 0;
	if (ConnectNamedPipe(m_hPipe,&m_ov))
		dwErr = ERROR_PIPE_CONNECTED;
	else
	{
		dwErr = GetLastError();
		if (dwErr == ERROR_IO_PENDING)
			dwErr = 0;
	}
	if (dwErr == ERROR_PIPE_CONNECTED)
	{
		dwErr = 0;
		if (!SetEvent(m_ov.hEvent))
			dwErr = GetLastError();
	}

	if (dwErr == 0)
	{
		++m_pProactor->m_outstanding;

		HANDLE hWait;
		if (!RegisterWaitForSingleObject(&hWait,m_ov.hEvent,accept_named_pipe2,this,INFINITE,WT_EXECUTEONLYONCE))
		{
			dwErr = GetLastError();
			--m_pProactor->m_outstanding;
		}
	}

	if (dwErr != 0)
		CloseHandle(m_hPipe.detach());
	
	return dwErr;
}

VOID CALLBACK PipeAcceptor::accept_named_pipe2(PVOID lpParameter, BOOLEAN /*TimerOrWaitFired*/)
{
	PipeAcceptor* pThis = static_cast<PipeAcceptor*>(lpParameter);
	
	pThis->do_accept();

	--pThis->m_pProactor->m_outstanding;
}

void PipeAcceptor::do_accept()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	if (m_closed)
	{
		SetEvent(m_hClosed);

		if (m_hPipe && m_hPipe != INVALID_HANDLE_VALUE)
		{
			CancelIo(m_hPipe);
			CloseHandle(m_hPipe.detach());
		}
		return;
	}

	DWORD dwErr = 0;
	DWORD dwBytes = 0;
	if (!GetOverlappedResult(m_hPipe,&m_ov,&dwBytes,TRUE))
		dwErr = GetLastError();

	OOBase::Win32::LocalSocket* pSocket = 0;
	if (dwErr == 0)
	{
		HANDLE hReadEvent = CreateEventW(NULL,TRUE,FALSE,NULL);
		if (!hReadEvent)
			dwErr = GetLastError();
		else
		{
			HANDLE hWriteEvent = CreateEventW(NULL,TRUE,FALSE,NULL);
			if (!hWriteEvent)
				dwErr = GetLastError();
			else
			{
				OOBASE_NEW(pSocket,OOBase::Win32::LocalSocket(m_hPipe,hReadEvent,hWriteEvent));
				if (!pSocket)
				{
					dwErr = ERROR_OUTOFMEMORY;
					CloseHandle(hWriteEvent);
				}
				else
					m_hPipe.detach();
			}
			
			if (dwErr != 0)
				CloseHandle(hReadEvent);
		}
	}
	
	// Call the acceptor
	bool again = m_sync_handler->on_accept(pSocket,dwErr) && (dwErr == 0);
	
	// Submit another accept if we want
	if (again)
	{
		dwErr = accept_named_pipe();
		if (dwErr != 0)
			m_sync_handler->on_accept(0,dwErr);
	}
}

OOBase::Socket* OOSvrBase::ProactorImpl::accept_local(Acceptor* handler, const std::string& path, int* perr, SECURITY_ATTRIBUTES* psa)
{
	*perr = 0;

	OOBase::SmartPtr<PipeAcceptor> pAcceptor = 0;
	OOBASE_NEW(pAcceptor,PipeAcceptor(this,"\\\\.\\pipe\\" + path,psa));
	if (!pAcceptor)
		*perr = ERROR_OUTOFMEMORY;
	else
	{
		*perr = pAcceptor->init(handler);
		if (*perr == 0)
			*perr = pAcceptor->accept_named_pipe();
	}

	if (*perr != 0)
		return 0;

	return pAcceptor.detach();
}

#endif // _WIN32
