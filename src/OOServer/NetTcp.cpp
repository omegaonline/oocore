///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer.h"

#include "./NetTcp.h"

using namespace Omega;
using namespace OTL;

OMEGA_DEFINE_OID(User,OID_TcpProtocolHandler,"{4924E463-06A4-483b-9DAD-8BFD83ADCBFC}");

namespace User
{
	class TcpStream :
		public ObjectBase,
		public IO::IStream
	{
	public:
		bool init(const ACE_SOCK_Stream& stream);
		
		BEGIN_INTERFACE_MAP(TcpStream)
			INTERFACE_ENTRY(IO::IStream)
		END_INTERFACE_MAP()

	private:
		ACE_SOCK_Stream m_stream;
				
	// IStream members
	public:
		void ReadBytes(uint32_t& cbBytes, byte_t* val);
		uint32_t WriteBytes(uint32_t cbBytes, const byte_t* val);
	};

	class TcpAsyncStream :
		public ObjectBase,
		public IO::IStream,
		public ACE_Service_Handler
	{
	public:
		virtual ~TcpAsyncStream();

		bool init(const ACE_SOCK_Stream& stream, IO::IAsyncStreamCallback* pCallback);
		
		BEGIN_INTERFACE_MAP(TcpAsyncStream)
			INTERFACE_ENTRY(IO::IStream)
		END_INTERFACE_MAP()

	private:
		ACE_Thread_Mutex                    m_lock;
		ACE_SOCK_Stream                     m_stream;
		ObjectPtr<IO::IAsyncStreamCallback> m_ptrCallback;
		ACE_Asynch_Read_Stream              m_reader;
		ACE_Asynch_Write_Stream             m_writer;

		void handle_read_stream(const ACE_Asynch_Read_Stream::Result& result);
		void handle_write_stream(const ACE_Asynch_Write_Stream::Result& result);
		
	// IStream members
	public:
		void ReadBytes(uint32_t& cbBytes, byte_t* val);
		uint32_t WriteBytes(uint32_t cbBytes, const byte_t* val);
	};
}

bool User::TcpStream::init(const ACE_SOCK_Stream& stream)
{
	m_stream = stream;

	return true;
}

void User::TcpStream::ReadBytes(uint32_t& cbBytes, byte_t* val)
{
	// Get the timeout
	ACE_Time_Value deadline = ACE_Time_Value::max_time;
	ObjectPtr<Remoting::ICallContext> ptrCC;
	ptrCC.Attach(Remoting::GetCallContext());
	if (ptrCC)
	{
		uint64_t secs = 0;
		int32_t usecs = 0;
		ptrCC->Deadline(secs,usecs);
		deadline = ACE_Time_Value(secs,usecs);
	}

	// Turn the deadline into a wait
	if (deadline != ACE_Time_Value::max_time)
	{
		ACE_Time_Value now = ACE_OS::gettimeofday();
		if (deadline <= now)
			OMEGA_THROW(ETIMEDOUT);

		deadline -= now;
	}

	// Recv
	ssize_t r = m_stream.recv(val,cbBytes,deadline == ACE_Time_Value::max_time ? 0 : &deadline);
	if (r == -1)
		OMEGA_THROW(ACE_OS::last_error());

	cbBytes = static_cast<uint32_t>(r);
}

uint32_t User::TcpStream::WriteBytes(uint32_t cbBytes, const byte_t* val)
{
	// Get the timeout
	ACE_Time_Value deadline = ACE_Time_Value::max_time;
	ObjectPtr<Remoting::ICallContext> ptrCC;
	ptrCC.Attach(Remoting::GetCallContext());
	if (ptrCC)
	{
		uint64_t secs = 0;
		int32_t usecs = 0;
		ptrCC->Deadline(secs,usecs);
		deadline = ACE_Time_Value(secs,usecs);
	}

	// Turn the deadline into a wait
	if (deadline != ACE_Time_Value::max_time)
	{
		ACE_Time_Value now = ACE_OS::gettimeofday();
		if (deadline <= now)
			OMEGA_THROW(ETIMEDOUT);

		deadline -= now;
	}

	// Send
	ssize_t s = m_stream.send(val,cbBytes,deadline == ACE_Time_Value::max_time ? 0 : &deadline);
	if (s == -1)
		OMEGA_THROW(ACE_OS::last_error());

	return static_cast<uint32_t>(s);
}

User::TcpAsyncStream::~TcpAsyncStream()
{
	void* MORE_HERE_NOW;
}

bool User::TcpAsyncStream::init(const ACE_SOCK_Stream& stream, IO::IAsyncStreamCallback* pCallback)
{
	if (!pCallback)
		OMEGA_THROW(L"Tcp protocol handler must be async!");

	if (m_reader.open(*this,stream.get_handle()) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("reader.open() failed")),false);

	if (m_writer.open(*this,stream.get_handle()) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("writer.open() failed")),false);
	
	m_stream = stream;
	m_ptrCallback = pCallback;

	return true;
}

void User::TcpAsyncStream::handle_read_stream(const ACE_Asynch_Read_Stream::Result& result)
{
	ACE_Message_Block& mb = result.message_block();

	bool bSuccess = (result.success() != 0);
	if (result.bytes_transferred())
	{
		try
		{
			OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

			if (m_ptrCallback)
			{
				size_t res = result.bytes_transferred();
				char* rd_pos = mb.rd_ptr();
				while (res)
				{
					uint32_t cbBytes = (uint32_t)-1;
					if (res < (uint32_t)-1)
						cbBytes = (uint32_t)res;

					m_ptrCallback->OnSignal(IO::IAsyncStreamCallback::Read,cbBytes,(const byte_t*)rd_pos);
					res -= cbBytes;
					rd_pos += cbBytes;
					mb.rd_ptr(rd_pos);
				};
			}
		}
		catch (IException* pE)
		{
			pE->Release();
			bSuccess = false;
		}
		catch (...)
		{
			bSuccess = false;
		}	
	}

	mb.release();

	if (!bSuccess)
	{
		int err = result.error();
		if (err != 0 && err != ENOTSOCK)
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("handle_read_stream() failed")));

		// Ignore the failure, we will try our best anyway!
		ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	
		m_stream.close();
		
		if (m_ptrCallback)
		{
			m_ptrCallback->OnSignal(IO::IAsyncStreamCallback::Closed,0,0);
			m_ptrCallback.Release();
		}
	}
}

void User::TcpAsyncStream::handle_write_stream(const ACE_Asynch_Write_Stream::Result& result)
{
	ACE_Message_Block& mb = result.message_block();

	bool bSuccess = (result.success() != 0);
	if (result.bytes_transferred())
	{
		try
		{
			OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

			if (m_ptrCallback)
			{
				size_t res = result.bytes_transferred();
				char* rd_pos = mb.base();
				while (res)
				{
					uint32_t cbBytes = (uint32_t)-1;
					if (res < (uint32_t)-1)
						cbBytes = (uint32_t)res;

					m_ptrCallback->OnSignal(IO::IAsyncStreamCallback::Written,cbBytes,(const byte_t*)rd_pos);
					res -= cbBytes;
					rd_pos += cbBytes;
				};
			}
		}
		catch (IException* pE)
		{
			pE->Release();
			bSuccess = false;
		}
		catch (...)
		{
			bSuccess = false;
		}	
	}

	mb.release();

	if (!bSuccess)
	{
		int err = result.error();
		if (err != 0 && err != ENOTSOCK)
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("handle_write_stream() failed")));

		// Ignore the failure, we will try our best anyway!
		ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	
		m_stream.close();
		
		if (m_ptrCallback)
		{
			m_ptrCallback->OnSignal(IO::IAsyncStreamCallback::Closed,0,0);
			m_ptrCallback.Release();
		}
	}
}

void User::TcpAsyncStream::ReadBytes(uint32_t& cbBytes, byte_t* /*val*/)
{
	// Create a new Message_Block
	ACE_Message_Block* mb = 0;
	OMEGA_NEW(mb,ACE_Message_Block(cbBytes));
	
	// Start an async read
	if (m_reader.read(*mb,cbBytes) != 0)
	{
		int err = ACE_OS::last_error();
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: read failed, code: %#x - %m\n"),err));
		mb->release();
		OMEGA_THROW(err);
	}

	// cbBytes is always 0, because we are always async
	cbBytes = 0;
}

uint32_t User::TcpAsyncStream::WriteBytes(uint32_t cbBytes, const byte_t* val)
{
	// Create a new Message_Block
	ACE_Message_Block* mb = 0;
	OMEGA_NEW(mb,ACE_Message_Block(cbBytes));

	// Copy the data to write
	if (mb->copy(reinterpret_cast<const char*>(val),cbBytes) != 0)
	{
		mb->release();
		OMEGA_THROW(ACE_OS::last_error());
	}
	
	// Start an async write
	if (m_writer.write(*mb,cbBytes) != 0)
	{
		int err = ACE_OS::last_error();
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: read failed, code: %#x - %m\n"),err));
		mb->release();
		OMEGA_THROW(err);
	}

	// Always 0, because we are always async
	return 0;
}

IO::IStream* User::TcpProtocolHandler::OpenStream(const string_t& strEndPoint, IO::IAsyncStreamCallback* pCallback)
{
	// First try to determine the protocol...
	size_t pos = strEndPoint.Find(L"://");
	if (pos == string_t::npos)
		OMEGA_THROW(L"No protocol specified!");

	// Create an address
	ACE_INET_Addr addr(ACE_TEXT_WCHAR_TO_TCHAR(strEndPoint.Mid(pos+3).c_str()),PF_INET);

	// Get the timeout
	ACE_Time_Value deadline = ACE_Time_Value::max_time;
	ObjectPtr<Remoting::ICallContext> ptrCC;
	ptrCC.Attach(Remoting::GetCallContext());
	if (ptrCC)
	{
		uint64_t secs = 0;
		int32_t usecs = 0;
		ptrCC->Deadline(secs,usecs);
		deadline = ACE_Time_Value(secs,usecs);
	}

	// Turn the deadline into a wait
	if (deadline != ACE_Time_Value::max_time)
	{
		ACE_Time_Value now = ACE_OS::gettimeofday();
		if (deadline <= now)
			OMEGA_THROW(ETIMEDOUT);

		deadline -= now;
	}

	// Connect!
	ACE_SOCK_Stream stream;
	if (ACE_SOCK_Connector().connect(stream,addr,deadline == ACE_Time_Value::max_time ? 0 : &deadline) != 0)
		OMEGA_THROW(ACE_OS::last_error());

	// Create a wrapper class around the stream
	if (pCallback)
	{
		ObjectPtr<ObjectImpl<TcpAsyncStream> > ptrStream = ObjectImpl<TcpAsyncStream>::CreateInstancePtr();
		if (!ptrStream->init(stream,pCallback))
			OMEGA_THROW(ACE_OS::last_error());
	
		return ptrStream.QueryInterface<IO::IStream>();
	}
	else
	{
		ObjectPtr<ObjectImpl<TcpStream> > ptrStream = ObjectImpl<TcpStream>::CreateInstancePtr();
		if (!ptrStream->init(stream))
			OMEGA_THROW(ACE_OS::last_error());
	
		return ptrStream.QueryInterface<IO::IStream>();
	}
}
