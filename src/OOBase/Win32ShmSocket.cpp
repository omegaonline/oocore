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

#include "Win32ShmSocket.h"
#include "Win32Socket.h"

#if defined(_WIN32)

namespace
{
	class ShmSocket : 
		public OOBase::Socket,
		public OOBase::Win32::ShmSocketImpl
	{
	public:
		virtual int send(const void* buf, size_t len, const OOBase::timeval_t* timeout = 0);
		virtual size_t recv(void* buf, size_t len, int* perr, const OOBase::timeval_t* timeout = 0);
		virtual void close();

	private:
		OOBase::SpinLock m_close_lock;
		OOBase::Mutex m_read_lock;
		OOBase::Mutex m_write_lock;
	};
}

void ShmSocket::close()
{
	// Acquire both locks...
	OOBase::Guard<OOBase::SpinLock> guard(m_close_lock);
	
	OOBase::Win32::ShmSocketImpl::close();
}

int ShmSocket::send(const void* buf, size_t len, const OOBase::timeval_t* timeout)
{
	assert(buf);

	if (!len)
		return 0;

	OOBase::timeval_t wait;
	if (timeout)
		wait = *timeout;

	OOBase::Countdown countdown(&wait);

	// Acquire the local mutex
	OOBase::Guard<OOBase::Mutex> guard(m_write_lock,false);
	if (!guard.acquire(timeout ? wait.msec() : 0))
		return ERROR_TIMEOUT;

	if (WaitForSingleObject(m_close_event,0) == WAIT_OBJECT_0)
		return ERROR_BROKEN_PIPE;

	Fifo& fifo = m_fifos[m_bServer ? 0 : 1];

	if (timeout)
		countdown.update();

	HANDLE handles[2] = 
	{
		m_close_event,
		fifo.m_write_event
	};
	
	// Wait if the fifo is full - this is a shared event
	if (fifo.is_full())
	{
		DWORD dwWait = WaitForMultipleObjects(2,handles,FALSE,timeout ? wait.msec() : INFINITE);
		if (dwWait == WAIT_OBJECT_0)
			return ERROR_BROKEN_PIPE;
		else if (dwWait == WAIT_TIMEOUT)
			return ERROR_TIMEOUT;
		else if (dwWait != WAIT_OBJECT_0+1)
			return GetLastError();
	}

	// Now we must loop to write the whole buffer...
	const char* data = static_cast<const char*>(buf);
	for (;;)
	{
		// Do the send
		int err = fifo.send_i(data,len);
		if (!len || err != 0)
			return err;

		// Now wait for more room...		
		// No timeout this time as we must write whole blocks
		DWORD dwWait = WaitForMultipleObjects(2,handles,FALSE,INFINITE);
		if (dwWait == WAIT_OBJECT_0)
			return ERROR_BROKEN_PIPE;
		else if (dwWait != WAIT_OBJECT_0+1)
			return GetLastError();
	}
}

size_t ShmSocket::recv(void* buf, size_t len, int* perr, const OOBase::timeval_t* timeout)
{
	assert(buf && perr);

	if (!len)
		return 0;

	OOBase::timeval_t wait;
	if (timeout)
		wait = *timeout;

	OOBase::Countdown countdown(&wait);

	// Acquire the local mutex
	OOBase::Guard<OOBase::Mutex> guard(m_read_lock,false);
	if (!guard.acquire(timeout ? wait.msec() : 0))
	{
		*perr = ERROR_TIMEOUT;
		return 0;
	}

	if (WaitForSingleObject(m_close_event,0) == WAIT_OBJECT_0)
	{
		*perr = ERROR_BROKEN_PIPE;
		return 0;
	}

	if (timeout)
		countdown.update();

	Fifo& fifo = m_fifos[m_bServer ? 1 : 0];

	HANDLE handles[2] = 
	{
		m_close_event,
		fifo.m_read_event
	};
	
	char* data = static_cast<char*>(buf);
	size_t total = 0;
	while (len)
	{
		if (timeout)
			countdown.update();

		// Wait if the fifo is empty - this is a shared event
		if (fifo.is_empty())
		{
			DWORD dwWait = WaitForMultipleObjects(2,handles,FALSE,timeout ? wait.msec() : INFINITE);
			if (dwWait == WAIT_TIMEOUT)
			{
				*perr = ERROR_TIMEOUT;
				return total;
			}
			else if (dwWait == WAIT_OBJECT_0)
			{
				*perr = ERROR_BROKEN_PIPE;
				return total;
			}
			else if (dwWait != WAIT_OBJECT_0+1)
			{
				*perr = GetLastError();
				return total;
			}
		}

		// Recv as much as we can...
		total += fifo.recv_i(data,len,perr);
		if (*perr != 0)
			break;
	}
	
	return total;
}

OOBase::Win32::ShmSocketImpl::~ShmSocketImpl() 
{
	close();

	if (m_fifos[0].m_shared)
		UnmapViewOfFile(m_fifos[0].m_shared);

	m_fifos[0].m_shared = 0;
	m_fifos[1].m_shared = 0;
}

bool OOBase::Win32::ShmSocketImpl::init_server(const std::string& strName, OOBase::LocalSocket* via, int* perr, const OOBase::timeval_t* timeout, SECURITY_ATTRIBUTES* psa)
{
	assert(perr);
	assert(!strName.empty());

	// We are the server
	m_bServer = true;

	// Create a new name...
	std::stringstream out;
	out << strName << "_SHM_" << this;
	std::string strNewName = out.str();
	
	// Create the shared memory
	m_hMapping = CreateFileMappingA(INVALID_HANDLE_VALUE,psa,PAGE_READWRITE,0,2*sizeof(Fifo::SharedInfo),strNewName.c_str());
	if (!m_hMapping || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		*perr = GetLastError();
		return false;
	}

	// Map the two fifos
	if (!create_fifos(perr,strNewName.c_str()))
		return false;
	
	// Send the name...
	size_t uLen = strNewName.length()+1;
	*perr = via->send(uLen,timeout);
	if (*perr == 0)
		*perr = via->send(strNewName.c_str(),uLen);

	// Bind the pipe
	*perr = bind_socket(via);
	if (*perr != 0)
		return false;

	return (*perr == 0);
}

bool OOBase::Win32::ShmSocketImpl::init_client(OOBase::LocalSocket* via, int* perr, const OOBase::timeval_t* timeout)
{
	// We are the client
	m_bServer = false;

	// Read the shared memory name length
	size_t uLen = 0;
	*perr = via->recv(uLen,timeout);
	if (*perr)
		return false;

	// Read the string
	OOBase::SmartPtr<char,OOBase::ArrayDestructor<char> > buf = 0;
	OOBASE_NEW(buf,char[uLen]);
	if (!buf)
	{
		*perr = ERROR_OUTOFMEMORY;
		return false;
	}

	via->recv(buf.value(),uLen,perr);
	if (*perr)
		return false;

	// Bind the pipe
	*perr = bind_socket(via);
	if (*perr != 0)
		return false;
		
	// Open the shared memory
	m_hMapping = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE,FALSE,buf.value());
	if (!m_hMapping)
	{
		*perr = GetLastError();
		return false;
	}

	// Map the two fifos
	return create_fifos(perr,buf.value());
}

bool OOBase::Win32::ShmSocketImpl::create_fifos(int* perr, const char* name)
{
	m_fifos[0].m_shared = static_cast<Fifo::SharedInfo*>(MapViewOfFile(m_hMapping,FILE_MAP_READ | FILE_MAP_WRITE,0,0,2*sizeof(Fifo::SharedInfo)));
	if (!m_fifos[0].m_shared)
	{
		*perr = GetLastError();
		return false;
	}

	m_fifos[1].m_shared = m_fifos[0].m_shared + 1;

	for (size_t index=0;index<2;++index)
	{
		std::stringstream out;
		out << name << "_M" << index;
		
		if (m_bServer)
			m_fifos[index].m_read_event = CreateEventA(NULL,FALSE,FALSE,out.str().c_str());
		else
			m_fifos[index].m_read_event = OpenEventA(EVENT_ALL_ACCESS,FALSE,out.str().c_str());

		if (!m_fifos[index].m_read_event || (m_bServer && GetLastError() == ERROR_ALREADY_EXISTS))
		{
			*perr = GetLastError();
			return false;
		}

		out << "W";
		if (m_bServer)
			m_fifos[index].m_write_event = CreateEventA(NULL,FALSE,FALSE,out.str().c_str());
		else
			m_fifos[index].m_write_event = OpenEventA(EVENT_ALL_ACCESS,FALSE,out.str().c_str());

		if (!m_fifos[index].m_write_event || (m_bServer && GetLastError() == ERROR_ALREADY_EXISTS))
		{
			*perr = GetLastError();
			return false;
		}
	}

	return true;
}

int OOBase::Win32::ShmSocketImpl::bind_socket(OOBase::LocalSocket* via)
{
	// Duplicate the pipe handle... we use this to detect close
	m_hPipe = static_cast<Win32::LocalSocket*>(via)->swap_out_handle();
	if (!m_hPipe)
		return GetLastError();

	// Start an overlapped read, this will fail when the pipe breaks...

	// Create the event
	m_close_event = CreateEventW(NULL,TRUE,FALSE,NULL);
	if (!m_close_event)
		return GetLastError();

	// Alloc the overlapped
	OOBASE_NEW(m_ptrOv,OV);
	if (!m_ptrOv)
		return ERROR_OUTOFMEMORY;

	memset(m_ptrOv.value(),0,sizeof(OV));
	m_ptrOv->hEvent = m_close_event;	

	// Start the read
	ReadFile(m_hPipe,&m_ptrOv->buf,0,NULL,static_cast<OVERLAPPED*>(m_ptrOv.value()));
	if (GetLastError() != ERROR_IO_PENDING)
		return GetLastError();
		
	return 0;
}

bool OOBase::Win32::ShmSocketImpl::Fifo::is_full()
{
	return ((m_shared->m_write_pos + 1) % SharedInfo::buffer_size == m_shared->m_read_pos);
}

bool OOBase::Win32::ShmSocketImpl::Fifo::is_empty()
{
	return (m_shared->m_write_pos == m_shared->m_read_pos);
}

size_t OOBase::Win32::ShmSocketImpl::Fifo::recv_i(char*& data, size_t& len, int* perr)
{
	// Count the total recvd
	size_t total = 0;

	// Stash write_pos because it may increase while we work
	size_t write_pos = m_shared->m_write_pos;
	
	// See if we need to signal that we have read
	bool writing_blocked = (((write_pos + 1) % Fifo::SharedInfo::buffer_size) == m_shared->m_read_pos);

	// If read is ahead of write, read until the end of the buffer
	if (m_shared->m_read_pos > write_pos)
	{
		// Read up to the end of the buffer
		size_t to_read = Fifo::SharedInfo::buffer_size - m_shared->m_read_pos;
		if (to_read > len)
			to_read = len;

		memcpy(data,m_shared->m_data + m_shared->m_read_pos,to_read);
		data += to_read;
		total += to_read;
		len -= to_read;
		m_shared->m_read_pos += to_read;
		
		// Make sure we wrap
		if (m_shared->m_read_pos == Fifo::SharedInfo::buffer_size)
			m_shared->m_read_pos = 0;
	}

	// Now read up to write...
	if (len)
	{
		// Write up to read_pos
		size_t to_read = write_pos - m_shared->m_read_pos;
		if (to_read > len)
			to_read = len;

		if (to_read)
		{
			memcpy(data,m_shared->m_data + m_shared->m_read_pos,to_read);
			data += to_read;
			total += to_read;
			len -= to_read;
			m_shared->m_read_pos += to_read;
		}
	}

	// Let the writers know we have read
	if (writing_blocked)
	{
		if (!SetEvent(m_write_event))
			*perr = GetLastError();
	}

	return total;
}

int OOBase::Win32::ShmSocketImpl::Fifo::send_i(const char*& data, size_t& len)
{
	// Stash read_pos because it may increase while we work
	size_t read_pos = m_shared->m_read_pos;
	
	// See if we need to signal that we have written
	bool reading_blocked = (read_pos == m_shared->m_write_pos);

	// If write is ahead of read, write until the end of the buffer
	if (m_shared->m_write_pos >= read_pos)
	{
		// Write up to the end of the buffer
		size_t to_write = SharedInfo::buffer_size - m_shared->m_write_pos;
		if (to_write > len)
			to_write = len;

		memcpy(m_shared->m_data + m_shared->m_write_pos,data,to_write);
		data += to_write;
		len -= to_write;
		m_shared->m_write_pos += to_write;
		
		// Make sure we wrap
		if (m_shared->m_write_pos == SharedInfo::buffer_size)
			m_shared->m_write_pos = 0;
	}

	// Now write up to read...
	if (len)
	{
		// Write up to read_pos-1
		size_t to_write = (read_pos-1) - m_shared->m_write_pos;
		if (to_write > len)
			to_write = len;

		if (to_write)
		{
			memcpy(m_shared->m_data + m_shared->m_write_pos,data,to_write);
			data += to_write;
			len -= to_write;
			m_shared->m_write_pos += to_write;
		}
	}
	
	// Let the readers know we have written
	if (reading_blocked)
	{
		if (!SetEvent(m_read_event))
			return GetLastError();
	}

	return 0;
}

void OOBase::Win32::ShmSocketImpl::close()
{
	// Close the pipe
	if (m_hPipe.is_valid())
	{
		HANDLE hPipe = m_hPipe.detach();
		CloseHandle(hPipe);

		// Wait for the event to be signalled...
		DWORD dw = 0;
		GetOverlappedResult(hPipe,m_ptrOv.value(),&dw,TRUE);
	}
}

OOBase::Socket* OOBase::LocalSocket::connect_shared_mem(LocalSocket* via, int* perr, const timeval_t* timeout)
{
	assert(perr);

	// Alloc a ShmSocket
	SmartPtr<ShmSocket> ptrSock;
	OOBASE_NEW(ptrSock,ShmSocket());
	if (!ptrSock)
	{
		*perr = ERROR_OUTOFMEMORY;
		return 0;
	}

	if (!ptrSock->init_client(via,perr,timeout))
		return 0;
	
	return ptrSock.detach();
}

#endif // _WIN32
