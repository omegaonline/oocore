///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
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

#include "./OOServer_User.h"
#include "./UserManager.h"
#include "./NetTcp.h"

using namespace Omega;
using namespace OTL;

OMEGA_DEFINE_OID(User,OID_TcpProtocolHandler,"{4924E463-06A4-483b-9DAD-8BFD83ADCBFC}");

namespace User
{
	class TcpStream :
		public ObjectBase,
		public Net::IConnectedStream
	{
	public:
		void open(const ACE_INET_Addr& addr);

		BEGIN_INTERFACE_MAP(TcpStream)
			INTERFACE_ENTRY(IO::IStream)
			INTERFACE_ENTRY(Net::IConnectedStream)
		END_INTERFACE_MAP()

	private:
		ACE_SOCK_Stream m_stream;

	// IStream members
	public:
		void ReadBytes(uint64_t& cbBytes, byte_t* val);
		void WriteBytes(const uint64_t& cbBytes, const byte_t* val);

	// IConnectedStream members
	public:
		string_t GetRemoteEndpoint();
		string_t GetLocalEndpoint();
	};

	class TcpAsyncStream :
		public ObjectBase,
		public Net::IConnectedStream
	{
	public:
		TcpAsyncStream();
		virtual ~TcpAsyncStream();

		void open(TcpHandler* pHandler, uint32_t stream_id);

		BEGIN_INTERFACE_MAP(TcpAsyncStream)
			INTERFACE_ENTRY(IO::IStream)
			INTERFACE_ENTRY(Net::IConnectedStream)
		END_INTERFACE_MAP()

	private:
		TcpHandler* m_pHandler;
		uint32_t    m_stream_id;

	// IStream members
	public:
		void ReadBytes(uint64_t& cbBytes, byte_t* val);
		void WriteBytes(const uint64_t& cbBytes, const byte_t* val);

	// IConnectedStream members
	public:
		string_t GetRemoteEndpoint();
		string_t GetLocalEndpoint();
	};
}

void User::TcpStream::open(const ACE_INET_Addr& addr)
{
	// Get the timeout
	ACE_Time_Value wait = ACE_Time_Value::max_time;
	ObjectPtr<Remoting::ICallContext> ptrCC;
	ptrCC.Attach(Remoting::GetCallContext());
	if (ptrCC)
	{
		uint32_t msecs = ptrCC->Timeout();
		if (msecs != (uint32_t)-1)
			wait = ACE_Time_Value(msecs / 1000,(msecs % 1000) * 1000);
	}

	// Connect!
	if (ACE_SOCK_Connector().connect(m_stream,addr,wait == ACE_Time_Value::max_time ? 0 : &wait) != 0)
		OMEGA_THROW(ACE_OS::last_error());
}

void User::TcpStream::ReadBytes(uint64_t& cbBytes, byte_t* val)
{
	// Get the timeout
	ACE_Time_Value wait = ACE_Time_Value::max_time;
	ObjectPtr<Remoting::ICallContext> ptrCC;
	ptrCC.Attach(Remoting::GetCallContext());
	if (ptrCC)
	{
		uint32_t msecs = ptrCC->Timeout();
		if (msecs != (uint32_t)-1)
			wait = ACE_Time_Value(msecs / 1000,(msecs % 1000) * 1000);
	}

	// Recv
	ssize_t r = m_stream.recv(val,(size_t)cbBytes,wait == ACE_Time_Value::max_time ? 0 : &wait);
	if (r == -1)
		OMEGA_THROW(ACE_OS::last_error());

	cbBytes = static_cast<uint64_t>(r);
}

void User::TcpStream::WriteBytes(const uint64_t& cbBytes, const byte_t* val)
{
	// Get the timeout
	ACE_Time_Value wait = ACE_Time_Value::max_time;
	ObjectPtr<Remoting::ICallContext> ptrCC;
	ptrCC.Attach(Remoting::GetCallContext());
	if (ptrCC)
	{
		uint32_t msecs = ptrCC->Timeout();
		if (msecs != (uint32_t)-1)
			wait = ACE_Time_Value(msecs / 1000,(msecs % 1000) * 1000);
	}

	// Send
	ssize_t s = m_stream.send(val,(size_t)cbBytes,wait == ACE_Time_Value::max_time ? 0 : &wait);
	if (s == -1)
		OMEGA_THROW(ACE_OS::last_error());
}

string_t User::TcpStream::GetRemoteEndpoint()
{
	ACE_INET_Addr addr;
	if (m_stream.get_remote_addr(addr) != 0)
		OMEGA_THROW(ACE_OS::last_error());

	ACE_TCHAR szBuf[1024];
	addr.addr_to_string(szBuf,1024,0);

	return string_t(ACE_TEXT_ALWAYS_WCHAR(szBuf));
}

string_t User::TcpStream::GetLocalEndpoint()
{
	ACE_INET_Addr addr;
	if (m_stream.get_local_addr(addr) != 0)
		OMEGA_THROW(ACE_OS::last_error());

	ACE_TCHAR szBuf[1024];
	addr.addr_to_string(szBuf,1024,0);

	return string_t(ACE_TEXT_ALWAYS_WCHAR(szBuf));
}

User::TcpAsyncStream::TcpAsyncStream() :
	m_pHandler(0), m_stream_id(0)
{
}

User::TcpAsyncStream::~TcpAsyncStream()
{
	if (m_pHandler && m_stream_id)
	{
		m_pHandler->AsyncClose(m_stream_id);
		m_pHandler->Release();
		m_pHandler = 0;
	}
}

void User::TcpAsyncStream::open(TcpHandler* pHandler, uint32_t stream_id)
{
	m_pHandler = pHandler;
	m_pHandler->AddRef();

	m_stream_id = stream_id;
}

void User::TcpAsyncStream::ReadBytes(uint64_t& cbBytes, byte_t* /*val*/)
{
	if (cbBytes > (size_t)-1)
		OMEGA_THROW(E2BIG);

	size_t len = static_cast<size_t>(cbBytes);

	// cbBytes is always 0, because we are always async
	cbBytes = 0;

	// Create a new Message_Block
	ACE_Message_Block* mb = 0;
	OMEGA_NEW(mb,ACE_Message_Block(len));

	try
	{
		m_pHandler->AsyncRead(m_stream_id,mb,len);
	}
	catch (...)
	{
		mb->release();
		throw;
	}
}

void User::TcpAsyncStream::WriteBytes(const uint64_t& cbBytes, const byte_t* val)
{
	if (cbBytes > (size_t)-1)
		OMEGA_THROW(E2BIG);

	size_t len = static_cast<size_t>(cbBytes);

	// Create a new Message_Block
	ACE_Message_Block* mb = 0;
	OMEGA_NEW(mb,ACE_Message_Block(len));

	// Copy the data to write
	if (mb->copy(reinterpret_cast<const char*>(val),len) != 0)
	{
		mb->release();
		OMEGA_THROW(ACE_OS::last_error());
	}

	try
	{
		m_pHandler->AsyncWrite(m_stream_id,mb);
	}
	catch (...)
	{
		mb->release();
		throw;
	}
}

string_t User::TcpAsyncStream::GetRemoteEndpoint()
{
	return m_pHandler->GetRemoteEndpoint(m_stream_id);
}

string_t User::TcpAsyncStream::GetLocalEndpoint()
{
	return m_pHandler->GetLocalEndpoint(m_stream_id);
}

void User::TcpHandler::TcpAsync::act(const void* pv)
{
	m_stream_id = (uint32_t)(size_t)pv;
}

void User::TcpHandler::TcpAsync::open(ACE_HANDLE new_handle, ACE_Message_Block&)
{
	m_stream = ACE_SOCK_Stream(new_handle);

	if (!m_pHandler->add_async(m_stream_id,this))
	{
		release();
		return;
	}

	bool bOk = false;
	if (m_reader.open(*this,new_handle) == 0 &&
		m_writer.open(*this,new_handle) == 0)
	{
		++m_refcount;
		if (!User::Manager::call_async_function(&open_stream_thunk,this))
		{
			m_pHandler->remove_async(m_stream_id);
			release();
			return;
		}

		bOk = true;
	}

	if (!bOk)
	{
		int err = ACE_OS::last_error();

		ACE_OutputCDR output;
		output << err;

		++m_refcount;
		if (!output.good_bit() || !Manager::call_async_function(&error_thunk,this,output.begin()))
		{
			m_pHandler->remove_async(m_stream_id);
			release();
		}
	}
}

void User::TcpHandler::TcpAsync::close()
{
	m_stream.close();
}

string_t User::TcpHandler::TcpAsync::local_endpoint()
{
	ACE_INET_Addr addr;
	if (m_stream.get_local_addr(addr) != 0)
		OMEGA_THROW(ACE_OS::last_error());

	ACE_TCHAR szBuf[1024];
	addr.addr_to_string(szBuf,1024,0);

	return string_t(ACE_TEXT_ALWAYS_WCHAR(szBuf));
}

string_t User::TcpHandler::TcpAsync::remote_endpoint()
{
	ACE_INET_Addr addr;
	if (m_stream.get_remote_addr(addr) != 0)
		OMEGA_THROW(ACE_OS::last_error());

	ACE_TCHAR szBuf[1024];
	addr.addr_to_string(szBuf,1024,0);

	return string_t(ACE_TEXT_ALWAYS_WCHAR(szBuf));
}

void User::TcpHandler::TcpAsync::handle_read_stream(const ACE_Asynch_Read_Stream::Result& result)
{
	ACE_Message_Block& mb = result.message_block();
	if (result.success() == 0 || result.bytes_transferred() == 0)
	{
		ACE_OutputCDR output;

		if (result.success() == 0)
			output << (int)result.error();
		else
			output << (int)ESHUTDOWN;

		++m_refcount;
		if (!output.good_bit() || !Manager::call_async_function(&error_thunk,this,output.begin()))
		{
			release();
			m_pHandler->remove_async(m_stream_id);
		}
	}
	else
	{
		++m_refcount;
		if (!Manager::call_async_function(&handle_read_stream_thunk,this,&mb))
		{
			release();
			m_stream.close();
		}
	}

	mb.release();
	release();
}

void User::TcpHandler::TcpAsync::handle_write_stream(const ACE_Asynch_Write_Stream::Result& result)
{
	ACE_Message_Block& mb = result.message_block();
	if (result.success() == 0)
	{
		ACE_OutputCDR output;
		output << (int)result.error();

		++m_refcount;
		if (!output.good_bit() || !Manager::call_async_function(&error_thunk,this,output.begin()))
		{
			release();
			m_pHandler->remove_async(m_stream_id);
		}
	}
	else
	{
		ACE_OutputCDR output;
		size_t len = result.bytes_transferred();
#ifdef OMEGA_64
		output << (ACE_CDR::ULongLong)len;
#else
		output << (ACE_CDR::ULong)len;
#endif

		++m_refcount;
		if (!output.good_bit() || !Manager::call_async_function(&handle_write_stream_thunk,this,output.begin()))
		{
			release();
			m_stream.close_writer();
		}
	}

	mb.release();
	release();
}

void User::TcpHandler::TcpAsync::error_thunk(void* pParam, ACE_InputCDR& input)
{
	TcpAsync* pThis = static_cast<TcpAsync*>(pParam);

	int err = -1;
	input >> err;

	try
	{
		if (!input.good_bit())
			OMEGA_THROW(ACE_OS::last_error());

		pThis->m_pHandler->OnAsyncError(pThis->m_stream_id,err);
	}
	catch (Omega::IException* pE)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%W: Unhandled exception: %W\n"),pE->GetSource().c_str(),pE->GetDescription().c_str()));

		pE->Release();
	}
	catch (...)
	{
	}

	pThis->release();
}

void User::TcpHandler::TcpAsync::open_stream_thunk(void* pParam, ACE_InputCDR&)
{
	TcpAsync* pThis = static_cast<TcpAsync*>(pParam);
	try
	{
		pThis->m_pHandler->OnAsyncOpen(pThis->m_stream_id);
	}
	catch (Omega::IException* pE)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%W: Unhandled exception: %W\n"),pE->GetSource().c_str(),pE->GetDescription().c_str()));

		pE->Release();
		pThis->m_stream.close();
	}
	catch (...)
	{
		pThis->m_stream.close();
	}

	pThis->release();
}

void User::TcpHandler::TcpAsync::handle_read_stream_thunk(void* pParam, ACE_InputCDR& input)
{
	TcpAsync* pThis = static_cast<TcpAsync*>(pParam);
	try
	{
		pThis->m_pHandler->OnAsyncRead(pThis->m_stream_id,input.start());
	}
	catch (Omega::IException* pE)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%W: Unhandled exception: %W\n"),pE->GetSource().c_str(),pE->GetDescription().c_str()));

		pE->Release();
		pThis->m_stream.close();
	}
	catch (...)
	{
		pThis->m_stream.close();
	}

	pThis->release();
}

void User::TcpHandler::TcpAsync::handle_write_stream_thunk(void* pParam, ACE_InputCDR& input)
{
#ifdef OMEGA_64
	ACE_CDR::ULongLong len = 0;
#else
	ACE_CDR::ULong len = 0;
#endif
	input >> len;

	TcpAsync* pThis = static_cast<TcpAsync*>(pParam);
	try
	{
		if (!input.good_bit())
			OMEGA_THROW(ACE_OS::last_error());

		pThis->m_pHandler->OnAsyncWrite(pThis->m_stream_id,len);
	}
	catch (Omega::IException* pE)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%W: Unhandled exception: %W\n"),pE->GetSource().c_str(),pE->GetDescription().c_str()));

		pE->Release();
		pThis->m_stream.close();
	}
	catch (...)
	{
		pThis->m_stream.close();
	}

	pThis->release();
}

bool User::TcpHandler::TcpAsync::read(ACE_Message_Block& mb, size_t len)
{
	++m_refcount;
	if (m_reader.read(mb,len) == 0)
		return true;

	release();
	return false;
}

bool User::TcpHandler::TcpAsync::write(ACE_Message_Block& mb)
{
	++m_refcount;
	if (m_writer.write(mb,mb.length()) == 0)
		return true;

	release();
	return false;
}

User::TcpHandler::TcpHandler() :
	m_nNextStream(1),
	m_refcount(1)
{
	m_connector.m_pHandler = this;
}

bool User::TcpHandler::add_async(uint32_t stream_id, TcpAsync* pAsync)
{
	// Add to the map
	ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

	try
	{
		std::map<uint32_t,AsyncEntry>::iterator i = m_mapAsyncs.find(stream_id);
		if (i == m_mapAsyncs.end())
			return false;

		if (i->second.pAsync)
			return false;

		// Already addref'ed
		i->second.pAsync = pAsync;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

void User::TcpHandler::remove_async(uint32_t stream_id)
{
	// This is called when a notification has failed - we just remove it...

	try
	{
		// Find and remove from the map...
		OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,AsyncEntry>::iterator i = m_mapAsyncs.find(stream_id);
		if (i != m_mapAsyncs.end())
		{
			if (i->second.pAsync)
				i->second.pAsync->release();

			m_mapAsyncs.erase(i);
		}
	}
	catch (...)
	{
	}
}

void User::TcpHandler::OnAsyncError(uint32_t stream_id, int err)
{
	ObjectPtr<IO::IAsyncStreamNotify> ptrNotify;
	{
		// Find and remove from the map...
		OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,AsyncEntry>::iterator i = m_mapAsyncs.find(stream_id);
		if (i != m_mapAsyncs.end())
		{
			ptrNotify = i->second.ptrNotify;

			if (i->second.pAsync)
				i->second.pAsync->release();

			m_mapAsyncs.erase(i);
		}
	}

	if (ptrNotify)
	{
		ObjectPtr<IException> ptrE;
		ptrE.Attach(ISystemException::Create(err));

		ptrNotify->OnError(ptrE);
	}
}

void User::TcpHandler::OnAsyncOpen(uint32_t stream_id)
{
	ObjectPtr<IO::IAsyncStreamNotify> ptrNotify;
	{
		// Find and remove from the map...
		OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,AsyncEntry>::iterator i = m_mapAsyncs.find(stream_id);
		if (i != m_mapAsyncs.end())
			ptrNotify = i->second.ptrNotify;
	}

	if (ptrNotify)
		ptrNotify->OnOpened();
}

void User::TcpHandler::OnAsyncRead(uint32_t stream_id, const ACE_Message_Block* mb)
{
	ObjectPtr<IO::IAsyncStreamNotify> ptrNotify;
	{
		// Find and remove from the map...
		OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,AsyncEntry>::iterator i = m_mapAsyncs.find(stream_id);
		if (i != m_mapAsyncs.end())
			ptrNotify = i->second.ptrNotify;
	}

	if (ptrNotify)
		ptrNotify->OnRead(mb->total_length(),(const byte_t*)mb->rd_ptr());
}

void User::TcpHandler::OnAsyncWrite(uint32_t stream_id, size_t len)
{
	ObjectPtr<IO::IAsyncStreamNotify> ptrNotify;
	{
		// Find and remove from the map...
		OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,AsyncEntry>::iterator i = m_mapAsyncs.find(stream_id);
		if (i != m_mapAsyncs.end())
			ptrNotify = i->second.ptrNotify;
	}

	if (ptrNotify)
		ptrNotify->OnWritten(len);
}

void User::TcpHandler::AsyncRead(uint32_t stream_id, ACE_Message_Block* mb, size_t len)
{
	try
	{
		// Find in the map...
		OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,AsyncEntry>::iterator i = m_mapAsyncs.find(stream_id);
		if (i == m_mapAsyncs.end())
			OMEGA_THROW(ENOTCONN);

		// Start an async read
		if (!i->second.pAsync->read(*mb,len))
		{
			int err = ACE_OS::last_error();
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: read failed, code: %#x - %m\n"),err));
			OMEGA_THROW(err);
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void User::TcpHandler::AsyncWrite(uint32_t stream_id, ACE_Message_Block* mb)
{
	try
	{
		// Find in the map...
		OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,AsyncEntry>::iterator i = m_mapAsyncs.find(stream_id);
		if (i == m_mapAsyncs.end())
			OMEGA_THROW(ENOTCONN);

		// Start an async write
		if (!i->second.pAsync->write(*mb))
		{
			int err = ACE_OS::last_error();
			ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: write failed, code: %#x - %m\n"),err));
			OMEGA_THROW(err);
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void User::TcpHandler::AsyncClose(uint32_t stream_id)
{
	remove_async(stream_id);
}

string_t User::TcpHandler::GetLocalEndpoint(uint32_t stream_id)
{
	try
	{
		// Find in the map...
		OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,AsyncEntry>::iterator i = m_mapAsyncs.find(stream_id);
		if (i == m_mapAsyncs.end())
			OMEGA_THROW(ENOTCONN);

		return i->second.pAsync->local_endpoint();
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

string_t User::TcpHandler::GetRemoteEndpoint(uint32_t stream_id)
{
	try
	{
		// Find in the map...
		OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,AsyncEntry>::iterator i = m_mapAsyncs.find(stream_id);
		if (i == m_mapAsyncs.end())
			OMEGA_THROW(ENOTCONN);

		return i->second.pAsync->remote_endpoint();
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void User::TcpHandler::AsyncConnector::call_error(void* pParam, ACE_InputCDR& input)
{
	User::TcpHandler* pThis = (User::TcpHandler*)pParam;

	try
	{
		uint32_t stream_id = 0;
		input >> stream_id;
		int err = -1;
		input >> err;

		if (!input.good_bit())
			OMEGA_THROW(ACE_OS::last_error());

		pThis->OnAsyncError(stream_id,err);
	}
	catch (IException* pE)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%W: Unhandled exception: %W\n"),pE->GetSource().c_str(),pE->GetDescription().c_str()));

		pE->Release();
	}
	catch (...)
	{
	}
}

void User::TcpHandler::AsyncConnector::handle_connect(const ACE_Asynch_Connect::Result& result)
{
	ACE_Asynch_Connector<TcpAsync>::handle_connect(result);

	if (!result.success())
	{
		uint32_t stream_id = (uint32_t)(size_t)result.act();

		ACE_OutputCDR output;
		output << stream_id;
		int err = result.error();
		output << err;

		if (!output.good_bit() || !Manager::call_async_function(&call_error,m_pHandler,output.begin()))
			m_pHandler->remove_async(stream_id);
	}
}

User::TcpHandler::TcpAsync* User::TcpHandler::AsyncConnector::make_handler()
{
	TcpAsync* handler = 0;
	ACE_NEW_RETURN(handler,TcpAsync(m_pHandler),0);
	return handler;
}

void User::TcpHandler::Start()
{
	OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	if (m_connector.open() != 0)
		OMEGA_THROW(ACE_OS::last_error());
}

void User::TcpHandler::Stop()
{
	try
	{
		// Copy and clear the map
		OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,AsyncEntry> map(m_mapAsyncs);
		m_mapAsyncs.clear();

		guard.release();

		for (std::map<uint32_t,AsyncEntry>::iterator i=map.begin();i!=map.end();++i)
		{
			if (i->second.pAsync)
			{
				i->second.pAsync->close();
				i->second.pAsync->release();
			}
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	/*ACE_Time_Value wait(0,100);
	for (;;)
	{
		OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		if (m_mapAsyncs.empty())
			break;

		guard.release();

		ACE_OS::sleep(wait);
	}*/
}

Net::IConnectedStream* User::TcpHandler::OpenStream(const string_t& strEndpoint, IO::IAsyncStreamNotify* pNotify)
{
	// First try to determine the protocol...
	size_t pos = strEndpoint.Find(L':');
	if (pos == string_t::npos)
		OMEGA_THROW(L"No protocol specified!");

	// Create an address
	ACE_INET_Addr addr(ACE_TEXT_WCHAR_TO_TCHAR(strEndpoint.Mid(pos+1).c_str()));

	// Create a wrapper class around the stream
	if (pNotify)
	{
		// Create an id
		uint32_t stream_id = 0;

		try
		{
			// Create new id and insert into map
			OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			stream_id = m_nNextStream++;
			while (!stream_id || m_mapAsyncs.find(stream_id) != m_mapAsyncs.end())
				stream_id = m_nNextStream++;

			AsyncEntry entry;
			entry.pAsync = 0;
			entry.ptrNotify = pNotify;

			m_mapAsyncs.insert(std::map<uint32_t,AsyncEntry>::value_type(stream_id,entry));
		}
		catch (std::exception& e)
		{
			OMEGA_THROW(e);
		}

		try
		{
			ObjectPtr<ObjectImpl<TcpAsyncStream> > ptrStream = ObjectImpl<TcpAsyncStream>::CreateInstancePtr();
			ptrStream->open(this,stream_id);

			// Start an async connect
			if (m_connector.connect(addr,(const ACE_INET_Addr&)ACE_Addr::sap_any,1,(const void*)(size_t)stream_id) != 0)
				OMEGA_THROW(ACE_OS::last_error());

			return ptrStream.AddRef();
		}
		catch (...)
		{
			OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			m_mapAsyncs.erase(stream_id);

			throw;
		}
	}
	else
	{
		ObjectPtr<ObjectImpl<TcpStream> > ptrStream = ObjectImpl<TcpStream>::CreateInstancePtr();
		ptrStream->open(addr);

		return ptrStream.QueryInterface<Net::IConnectedStream>();
	}
}

User::TcpProtocolHandler::TcpProtocolHandler() :
	m_pHandler(0)
{
}

User::TcpProtocolHandler::~TcpProtocolHandler()
{
	Stop();
}

void User::TcpProtocolHandler::Start()
{
	OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

	if (m_pHandler)
		OMEGA_THROW(L"Service already started");

	OMEGA_NEW(m_pHandler,TcpHandler);

	m_pHandler->Start();
}

void User::TcpProtocolHandler::Stop()
{
	TcpHandler* pHandler;

	OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

	pHandler = m_pHandler;
	m_pHandler = 0;

	guard.release();

	if (pHandler)
	{
		pHandler->Stop();
		pHandler->Release();
	}
}

Net::IConnectedStream* User::TcpProtocolHandler::OpenStream(const string_t& strEndpoint, IO::IAsyncStreamNotify* pNotify)
{
	TcpHandler* pHandler;

	OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

	if (!m_pHandler)
		OMEGA_THROW(L"Service has been stopped!");

	pHandler = m_pHandler;
	pHandler->AddRef();

	guard.release();

	Net::IConnectedStream* pRet = pHandler->OpenStream(strEndpoint,pNotify);

	pHandler->Release();

	return pRet;
}
