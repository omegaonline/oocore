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
		virtual ~AsyncStreamCallback();
		
		void init(Manager* pManager, uint32_t channel_id, IO::IAsyncStreamCallback* pCallback);

		BEGIN_INTERFACE_MAP(AsyncStreamCallback)
			INTERFACE_ENTRY(IO::IAsyncStreamCallback)
		END_INTERFACE_MAP()

	private:
		Manager*                  m_pManager;
		IO::IAsyncStreamCallback* m_pCallback;
		uint32_t                  m_channel_id;

		void OnRead(uint32_t cbBytes, const byte_t* pData);
		void OnWritten(uint32_t cbBytes, const byte_t* pData);
		void OnClosed();

	// IAsyncStreamCallback members
	public:
		void OnSignal(IO::IAsyncStreamCallback::SignalType_t type, uint32_t cbBytes, const byte_t* pData);
	};
}

User::AsyncStreamCallback::AsyncStreamCallback() :
	m_pManager(0),
	m_pCallback(0),
	m_channel_id(0)
{
}

User::AsyncStreamCallback::~AsyncStreamCallback()
{
	if (m_channel_id)
	{
		// Remove from the map...
		OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_pManager->m_lock);

		m_pManager->m_mapStreamCallbacks.erase(m_channel_id);
	}
}

void User::AsyncStreamCallback::init(Manager* pManager, uint32_t channel_id, IO::IAsyncStreamCallback* pCallback)
{
	m_pManager = pManager;
	m_pCallback = pCallback;
	m_channel_id = channel_id;
}

void User::AsyncStreamCallback::OnSignal(IO::IAsyncStreamCallback::SignalType_t type, uint32_t cbBytes, const byte_t* pData)
{
	switch (type)
	{
	case IO::IAsyncStreamCallback::Read:
		OnRead(cbBytes,pData);
		break;

	case IO::IAsyncStreamCallback::Written:
		OnWritten(cbBytes,pData);
		break;

	case IO::IAsyncStreamCallback::Closed:
		OnClosed();
		break;

	default:
		break;
	}
}

void User::AsyncStreamCallback::OnRead(uint32_t cbBytes, const byte_t* pData)
{
	void* MORE_HERE_NOW;
}

void User::AsyncStreamCallback::OnWritten(uint32_t cbBytes, const byte_t* pData)
{
	void* MORE_HERE_NOW;
}

void User::AsyncStreamCallback::OnClosed()
{
	void* MORE_HERE_NOW;
}

IO::IStream* User::Manager::open_stream(const string_t& strEndPoint, IO::IAsyncStreamCallback* pCallback)
{
	return USER_MANAGER::instance()->open_stream_i(strEndPoint,pCallback);
}

IO::IStream* User::Manager::open_stream_i(const string_t& strEndPoint, IO::IAsyncStreamCallback* pCallback)
{
	// First try to determine the protocol...
	size_t pos = strEndPoint.Find(L"://");
	if (pos == string_t::npos)
		OMEGA_THROW(L"No protocol specified!");

	// Look up handler in registry
	string_t strProtocol = strEndPoint.Left(pos).ToLower();
	
	guid_t oid = guid_t::Null();
	ObjectPtr<Omega::Registry::IRegistryKey> ptrKey(L"\\Local User");
	if (ptrKey->IsSubKey(L"Networking\\Protocols\\" + strProtocol))
	{
		ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\" + strProtocol);
		if (ptrKey->IsValue(L"ServerHandlerOID"))
			oid = guid_t::FromString(ptrKey->GetStringValue(L"ServerHandlerOID"));
	}
	
	if (oid == guid_t::Null())
	{
		ptrKey = ObjectPtr<Omega::Registry::IRegistryKey>(L"\\");
		if (ptrKey->IsSubKey(L"Networking\\Protocols\\" + strProtocol))
		{
			ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\" + strProtocol);
			if (ptrKey->IsValue(L"ServerHandlerOID"))
				oid = guid_t::FromString(ptrKey->GetStringValue(L"ServerHandlerOID"));
		}
	}
	
	if (oid == guid_t::Null())
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

	// Open the stream...
	ObjectPtr<IO::IStream> ptrStream;
	uint32_t channel_id = 0;

	if (pCallback)
	{
		// Create a new stream id and insert into the map
		try
		{
			// Add to the map...
			OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			do
			{
				channel_id = ++m_nNextStreamCallback;
				if (channel_id & m_root_channel)
				{
					// Don't cross into pipe channels...
					channel_id = 0;
					m_nNextStreamCallback = 0;
				}
			} while (!channel_id || m_mapStreamCallbacks.find(channel_id) != m_mapStreamCallbacks.end());

			m_mapStreamCallbacks.insert(std::map<uint32_t,ObjectPtr<IO::IAsyncStreamCallback> >::value_type(channel_id,pCallback));
		}
		catch (std::exception& e)
		{
			OMEGA_THROW(e);
		}

		try
		{
			// Create a callback handler...
			ObjectPtr<ObjectImpl<AsyncStreamCallback> > ptrCallback = ObjectImpl<AsyncStreamCallback>::CreateInstancePtr();

			// Add to internal map
			ptrCallback->init(this,channel_id,pCallback);

			// Open with our callback
			ptrStream.Attach(ptrHandler->OpenStream(strEndPoint,ptrCallback));
		}
		catch (...)
		{
			if (channel_id)
			{
				// Remove from map!
				OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

				m_mapStreamCallbacks.erase(channel_id);
			}
			throw;
		}
	}
	else
	{
		// Open with no callback
		ptrStream.Attach(ptrHandler->OpenStream(strEndPoint,0));
	}

	return ptrStream.AddRef();
}

bool User::Manager::route_off(ACE_CDR::ULong dest_channel_id, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort dest_thread_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs, const ACE_Message_Block* mb)
{
	void* TICKET_92;

	return MessageHandler::route_off(dest_channel_id,src_channel_id,dest_thread_id,src_thread_id,deadline,attribs,mb);
}
