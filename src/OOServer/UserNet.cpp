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

#include "./UserManager.h"
#include "./NetTcp.h"
#include "./NetHttp.h"

using namespace Omega;
using namespace OTL;

namespace User
{
	class AsyncStreamCallback :
		public ObjectBase,
		public IO::IAsyncStreamCallback
	{
	public:
		AsyncStreamCallback();
		
		void init(Manager* pManager, IO::IStream* pStream, uint32_t channel_id);

		BEGIN_INTERFACE_MAP(AsyncStreamCallback)
			INTERFACE_ENTRY(IO::IAsyncStreamCallback)
		END_INTERFACE_MAP()

	private:
		ACE_Thread_Mutex m_lock;
		Manager*         m_pManager;
		IO::IStream*     m_pStream;
		uint32_t         m_cbBytesPending;
		uint32_t         m_channel_id;

		void ReadPending(uint32_t cbBytes);
		void Closed();

	// IAsyncStreamCallback members
	public:
		void OnSignal(IO::IAsyncStreamCallback::SignalType_t type, uint32_t cbBytes);
	};
}

User::AsyncStreamCallback::AsyncStreamCallback() :
	m_pManager(0),
	m_pStream(0),
	m_cbBytesPending(0),
	m_channel_id(0)
{
}

void User::AsyncStreamCallback::init(Manager* pManager, IO::IStream* pStream, uint32_t channel_id)
{
	OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

	m_pManager = pManager;
	m_pStream = pStream;
	m_channel_id = channel_id;

	if (m_cbBytesPending)
	{
		// We have something ready for us already!
		ReadPending(m_cbBytesPending);
	}
}

void User::AsyncStreamCallback::OnSignal(IO::IAsyncStreamCallback::SignalType_t type, uint32_t cbBytes)
{
	OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

	switch (type)
	{
	case IO::IAsyncStreamCallback::ReadPending:
		if (!m_pStream)
			m_cbBytesPending += cbBytes;
		else
			ReadPending(cbBytes);
		break;

	case IO::IAsyncStreamCallback::Closed:
		Closed();
		break;

	default:
		break;
	}
}

void User::AsyncStreamCallback::ReadPending(uint32_t cbBytes)
{
	void* MORE_HERE_NOW;
}

void User::AsyncStreamCallback::Closed()
{
	void* MORE_HERE_NOW;
}

uint32_t User::Manager::open_stream(const string_t& strEndPoint)
{
	return USER_MANAGER::instance()->open_stream_i(strEndPoint);
}

uint32_t User::Manager::open_stream_i(const string_t& strEndPoint)
{
	// First try to determine the protocol...
	size_t pos = strEndPoint.Find(L"://");
	if (pos == string_t::npos)
		OMEGA_THROW(L"No protocol specified!");

	// Look up handler in registry
	string_t strProtocol = strEndPoint.Left(pos).ToLower();
	
	guid_t oid;
	ObjectPtr<Omega::Registry::IRegistryKey> ptrKey(L"\\Server");
	if (ptrKey->IsSubKey(L"Networking\\ProtocolHandlers\\" + strProtocol))
	{
		ptrKey = ptrKey.OpenSubKey(L"Networking\\ProtocolHandlers\\" + strProtocol);
		oid = guid_t::FromString(ptrKey->GetStringValue(L"OID"));
	}
	else
	{
		if (strProtocol == L"tcp")
			oid = OID_TcpProtocolHandler;
		else if (strProtocol == L"http")
			oid = OID_HttpProtocolHandler;
		else
			OMEGA_THROW(L"No handler for protocol " + strProtocol);		
	}

	// Create the handler...
	ObjectPtr<IO::IProtocolHandler> ptrHandler(oid);

	// Create a callback handler...
	ObjectPtr<ObjectImpl<AsyncStreamCallback> > ptrCallback = ObjectImpl<AsyncStreamCallback>::CreateInstancePtr();

	// Open the stream...
	ObjectPtr<IO::IStream> ptrStream;
	ptrStream.Attach(ptrHandler->OpenStream(strEndPoint.Mid(pos+3),ptrCallback));

	// Create a new stream id and insert into the map
	uint32_t channel_id = 0;
	try
	{
		// Add to the map...
		OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		do
		{
			channel_id = ++m_nNextStream;
			if (channel_id & m_root_channel)
			{
				// Don't cross into pipe channels...
				channel_id = 0;
				m_nNextStream = 0;
			}
		} while (!channel_id || m_mapStreams.find(channel_id) != m_mapStreams.end());

		m_mapStreams.insert(std::map<uint32_t,ObjectPtr<IO::IStream> >::value_type(channel_id,ptrStream));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	try
	{
		// Add to internal map
		ptrCallback->init(this,ptrStream,channel_id);
	}
	catch (...)
	{
		// Remove from map!
		OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		m_mapStreams.erase(channel_id);

		throw;
	}

	return channel_id;
}

bool User::Manager::route_off(ACE_CDR::ULong dest_channel_id, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort dest_thread_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs, const ACE_Message_Block* mb)
{
	ObjectPtr<IO::IStream> ptrStream;
	try
	{
		// Add to the map...
		OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,ObjectPtr<IO::IStream> >::iterator i = m_mapStreams.find(dest_channel_id);
		if (i != m_mapStreams.end())
			ptrStream = i->second;
	}
	catch (std::exception&)
	{
		ACE_OS::last_error(EINVAL);
		return false;
	}

	if (ptrStream)
	{
		void* MORE_HERE_NOW;

		ACE_Message_Block* mb2 = mb->duplicate();

		try
		{
			size_t cbToWrite = mb->total_length();
			while (cbToWrite)
			{
				uint32_t cb = (uint32_t)cbToWrite;
				if (cbToWrite > (uint32_t)-1)
					cb = (uint32_t)-1;

				cb = ptrStream->WriteBytes(cb,(byte_t*)mb2->rd_ptr());
				cbToWrite -= cb;
				mb2->rd_ptr(cb);
			};
		}
		catch (...)
		{
			mb2->release();
			throw;
		}

		mb2->release();
	}

	return MessageHandler::route_off(dest_channel_id,src_channel_id,dest_thread_id,src_thread_id,deadline,attribs,mb);
}
