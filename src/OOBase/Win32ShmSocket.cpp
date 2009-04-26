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

#if defined(_WIN32)

namespace
{
	class ShmSocket : 
		public OOBase::Socket,
		public OOBase::Win32::ShmSocketImpl
	{
	public:
		bool init_server(const std::string& strName, OOBase::LocalSocket* via, int* perr, const OOBase::timeval_t* timeout);
		bool init_client(OOBase::LocalSocket* via, int* perr, const OOBase::timeval_t* timeout);
		
		virtual int send(const void* buf, size_t len, const OOBase::timeval_t* timeout = 0);
		virtual size_t recv(void* buf, size_t len, int* perr, const OOBase::timeval_t* timeout = 0);
		virtual void close();
	};
}

bool ShmSocket::init_server(const std::string& strName, OOBase::LocalSocket* via, int* perr, const OOBase::timeval_t* wait)
{
	return OOBase::Win32::ShmSocketImpl::init_server(strName,via,perr,wait);
}

bool ShmSocket::init_client(OOBase::LocalSocket* via, int* perr, const OOBase::timeval_t* wait)
{
	return OOBase::Win32::ShmSocketImpl::init_client(via,perr,wait);
}

void ShmSocket::close()
{
}

int ShmSocket::send(const void* buf, size_t len, const OOBase::timeval_t* timeout)
{
	assert(buf);

	if (!len)
		return 0;

	size_t index = m_bServer ? 0 : 1;

	OOBase::timeval_t wait;
	if (timeout)
		wait = *timeout;

	OOBase::Countdown countdown(&wait);

	// Acquire the local mutex
	OOBase::Guard<OOBase::Mutex> guard(m_fifos[index].m_lock,false);
	if (!guard.acquire(timeout ? wait.msec() : 0))
		return ERROR_TIMEOUT;

	if (timeout)
		countdown.update();

	// Wait if the fifo is full - this is a shared mutex
	if ((m_fifos[index].m_shared->m_write_pos + 1) % Fifo::SharedInfo::buffer_size == m_fifos[index].m_shared->m_read_pos)
	{
		DWORD dwWait = WaitForSingleObject(m_fifos[index].m_write_event,timeout ? wait.msec() : 0);
		if (dwWait == WAIT_TIMEOUT)
			return ERROR_TIMEOUT;
	}

	// Now we must loop to write the whole buffer...
	const char* data = static_cast<const char*>(buf);
	while (len)
	{
		// Stash read_pos because it may increase while we work
		size_t read_pos = m_fifos[index].m_shared->m_read_pos;
		
		// See if we need to signal that we have written
		bool reading_blocked = (read_pos == m_fifos[index].m_shared->m_write_pos);

		// If write is ahead of read, write until the end of the buffer
		if (m_fifos[index].m_shared->m_write_pos >= read_pos)
		{
			// Write up to the end of the buffer
			size_t to_write = Fifo::SharedInfo::buffer_size - m_fifos[index].m_shared->m_write_pos;
			if (to_write > len)
				to_write = len;

			memcpy(m_fifos[index].m_shared->m_data + m_fifos[index].m_shared->m_write_pos,data,to_write);
			data += to_write;
			len -= to_write;
			m_fifos[index].m_shared->m_write_pos += to_write;
			
			// Make sure we wrap
			if (m_fifos[index].m_shared->m_write_pos == Fifo::SharedInfo::buffer_size)
				m_fifos[index].m_shared->m_write_pos = 0;
		}

		// Now write up to read...
		if (len)
		{
			// Write up to read_pos-1
			size_t to_write = (read_pos-1) - m_fifos[index].m_shared->m_write_pos;
			if (to_write > len)
				to_write = len;

			if (to_write)
			{
				memcpy(m_fifos[index].m_shared->m_data + m_fifos[index].m_shared->m_write_pos,data,to_write);
				data += to_write;
				len -= to_write;
				m_fifos[index].m_shared->m_write_pos += to_write;
			}
		}

		// Let the readers know we have written
		if (reading_blocked)
		{
			if (!ReleaseMutex(m_fifos[index].m_read_event))
				return GetLastError();
		}

		// Now wait for more room if we need to
		if (len)
		{
			if ((m_fifos[index].m_shared->m_write_pos + 1) % Fifo::SharedInfo::buffer_size == m_fifos[index].m_shared->m_read_pos)
			{
				// No timeout this time as we must write whole blocks
				if (WaitForSingleObject(m_fifos[index].m_write_event,INFINITE) != WAIT_OBJECT_0)
					return GetLastError();
			}
		}
	}

	// By the time we get here we have written everything...
	return 0;
}

size_t ShmSocket::recv(void* buf, size_t len, int* perr, const OOBase::timeval_t* timeout)
{
	assert(buf && perr);

	if (!len)
		return 0;

	size_t index = m_bServer ? 1 : 0;

	OOBase::timeval_t wait;
	if (timeout)
		wait = *timeout;

	OOBase::Countdown countdown(&wait);

	// Acquire the local mutex
	OOBase::Guard<OOBase::Mutex> guard(m_fifos[index].m_lock,false);
	if (!guard.acquire(timeout ? wait.msec() : 0))
	{
		*perr = ERROR_TIMEOUT;
		return 0;
	}

	if (timeout)
		countdown.update();

	// Wait if the fifo is empty - this is a shared mutex
	if (m_fifos[index].m_shared->m_write_pos == m_fifos[index].m_shared->m_read_pos)
	{
		DWORD dwWait = WaitForSingleObject(m_fifos[index].m_read_event,timeout ? wait.msec() : 0);
		if (dwWait == WAIT_TIMEOUT)
		{
			*perr = ERROR_TIMEOUT;
			return 0;
		}
	}

	char* data = static_cast<char*>(buf);
	size_t len2 = 0;
	
	// Stash write_pos because it may increase while we work
	size_t write_pos = m_fifos[index].m_shared->m_write_pos;
	
	// See if we need to signal that we have written
	bool writing_blocked = (((write_pos + 1) % Fifo::SharedInfo::buffer_size) == m_fifos[index].m_shared->m_read_pos);

	// If read is ahead of write, read until the end of the buffer
	if (m_fifos[index].m_shared->m_read_pos > write_pos)
	{
		// Read up to the end of the buffer
		size_t to_read = Fifo::SharedInfo::buffer_size - m_fifos[index].m_shared->m_read_pos;
		if (to_read > len)
			to_read = len;

		memcpy(data,m_fifos[index].m_shared->m_data + m_fifos[index].m_shared->m_read_pos,to_read);
		data += to_read;
		len2 += to_read;
		m_fifos[index].m_shared->m_read_pos += to_read;
		
		// Make sure we wrap
		if (m_fifos[index].m_shared->m_read_pos == Fifo::SharedInfo::buffer_size)
			m_fifos[index].m_shared->m_read_pos = 0;
	}

	// Now read up to write...
	if (len2 < len)
	{
		// Write up to read_pos
		size_t to_read = m_fifos[index].m_shared->m_read_pos - write_pos;
		if (to_read > (len - len2))
			to_read = (len - len2);

		if (to_read)
		{
			memcpy(data,m_fifos[index].m_shared->m_data + m_fifos[index].m_shared->m_read_pos,to_read);
			data += to_read;
			len2 += to_read;
			m_fifos[index].m_shared->m_read_pos += to_read;
		}
	}

	// Let the writers know we have read
	if (writing_blocked)
	{
		if (!ReleaseMutex(m_fifos[index].m_write_event))
			return GetLastError();
	}

	return len2;
}

bool OOBase::Win32::ShmSocketImpl::init_server(const std::string& strName, OOBase::LocalSocket* via, int* perr, const OOBase::timeval_t* timeout)
{
	assert(perr);

	// We are the server
	m_bServer = false;

	// Create a new name...
	std::string strNewName = strName + "_SHM";

	// Send the name...
	size_t uLen = strNewName.length()+1;
	*perr = via->send(uLen,timeout);
	if (*perr == 0)
		*perr = via->send(strNewName.c_str(),uLen);

	if (*perr != 0)
		return false;

	// Create the shared memory
	m_hMapping = CreateFileMappingA(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,2*sizeof(Fifo::SharedInfo),strNewName.c_str());
	if (!m_hMapping)
	{
		*perr = GetLastError();
		return 0;
	}

	// Map the two fifos
	if (!create_fifo(0,perr,strNewName.c_str()))
		return false;

	return create_fifo(1,perr,strNewName.c_str());

}

bool OOBase::Win32::ShmSocketImpl::init_client(OOBase::LocalSocket* via, int* perr, const OOBase::timeval_t* timeout)
{
	// We are the client
	m_bServer = false;

	// Read the shared memory name length
	size_t uLen = 0;
	*perr = via->recv(uLen,timeout);
	if (*perr)
		return 0;

	// Read the string
	OOBase::SmartPtr<char,OOBase::ArrayDestructor<char> > buf = 0;
	OOBASE_NEW(buf,char[uLen]);
	if (!buf)
	{
		*perr = ERROR_OUTOFMEMORY;
		return 0;
	}

	via->recv(buf.value(),uLen,perr);
	if (perr)
		return 0;
		
	// Open the shared memory
	m_hMapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS,FALSE,buf.value());
	if (!m_hMapping)
	{
		*perr = GetLastError();
		return 0;
	}

	// Map the two fifos
	if (!create_fifo(0,perr,buf.value()))
		return false;

	return create_fifo(1,perr,buf.value());
}

bool OOBase::Win32::ShmSocketImpl::create_fifo(size_t index, int* perr, const char* name)
{
	assert(index == 0 || index == 1);

	if (index == 0)
		m_fifos[index].m_shared = static_cast<Fifo::SharedInfo*>(MapViewOfFile(m_hMapping,FILE_MAP_READ | FILE_MAP_WRITE,0,0,sizeof(Fifo::SharedInfo)));
	else
		m_fifos[index].m_shared = static_cast<Fifo::SharedInfo*>(MapViewOfFile(m_hMapping,FILE_MAP_READ | FILE_MAP_WRITE,0,sizeof(Fifo::SharedInfo),sizeof(Fifo::SharedInfo)));

	if (!m_fifos[index].m_shared)
	{
		*perr = GetLastError();
		return false;
	}

	char szBuf[MAX_PATH] = {0};
	sprintf_s(szBuf,sizeof(szBuf),"%s_R%u",name,index);

	if (m_bServer)
		m_fifos[index].m_read_event = CreateEventA(NULL,FALSE,FALSE,szBuf);
	else
		m_fifos[index].m_read_event = OpenEventA(EVENT_ALL_ACCESS,FALSE,szBuf);

	if (!m_fifos[index].m_read_event)
	{
		*perr = GetLastError();
		return false;
	}

	sprintf_s(szBuf,sizeof(szBuf),"%s_W%u",name,index);

	if (m_bServer)
		m_fifos[index].m_write_event = CreateEventA(NULL,FALSE,FALSE,szBuf);
	else
		m_fifos[index].m_write_event = OpenEventA(EVENT_ALL_ACCESS,FALSE,szBuf);

	if (!m_fifos[index].m_write_event)
	{
		*perr = GetLastError();
		return false;
	}

	return true;
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
