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

#include "./NetHttp.h"
#include "./NetTcp.h"

using namespace Omega;
using namespace OTL;

OMEGA_DEFINE_OID(User,OID_HttpProtocolHandler,"{EDB0676F-70B0-4e49-AACC-E8478F615277}");

namespace User
{
	class HttpCallback :
		public ObjectBase,
		public IO::IAsyncStreamCallback,
		public IO::IStream
	{
	public:
		HttpCallback();
		
		void init(IO::IStream* pStream, IO::IAsyncStreamCallback* pCallback);

		BEGIN_INTERFACE_MAP(HttpCallback)
			INTERFACE_ENTRY(IO::IAsyncStreamCallback)
			INTERFACE_ENTRY(IO::IStream)
		END_INTERFACE_MAP()

	private:
		ACE_Thread_Mutex                    m_lock;
		ObjectPtr<IO::IAsyncStreamCallback> m_ptrCallback;
		IO::IStream*                        m_pStream;
		uint32_t                            m_cbBytesPending;
		
		void ReadPending(uint32_t cbBytes);
		void Closed();

	// IAsyncStreamCallback members
	public:
		void OnSignal(IO::IAsyncStreamCallback::SignalType_t type, uint32_t cbBytes);

	// IStream members
	public:
		void ReadBytes(uint32_t& cbBytes, byte_t* val);
		uint32_t WriteBytes(uint32_t cbBytes, const byte_t* val);
	};
}

User::HttpCallback::HttpCallback() :
	m_pStream(0),
	m_cbBytesPending(0)
{
}

void User::HttpCallback::init(IO::IStream* pStream, IO::IAsyncStreamCallback* pCallback)
{
	OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

	m_ptrCallback = pCallback;
	m_pStream = pStream;

	if (m_cbBytesPending)
	{
		// We have something ready for us already!
		ReadPending(m_cbBytesPending);
	}
}

void User::HttpCallback::OnSignal(IO::IAsyncStreamCallback::SignalType_t type, uint32_t cbBytes)
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

void User::HttpCallback::ReadPending(uint32_t cbBytes)
{
	// This is where we parse HTTP response send on the results...
	void* MORE_HERE_NOW;
}

void User::HttpCallback::Closed()
{
	void* MORE_HERE_NOW;
}

void User::HttpCallback::ReadBytes(uint32_t& cbBytes, byte_t* val)
{
	OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);
}

uint32_t User::HttpCallback::WriteBytes(uint32_t cbBytes, const byte_t* val)
{
	OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

	return 0;
}

IO::IStream* User::HttpProtocolHandler::OpenStream(const string_t& strEndPoint, IO::IAsyncStreamCallback* pCallback)
{
	// First try to determine the protocol...
	size_t pos = strEndPoint.Find(L"://");
	if (pos == string_t::npos)
		OMEGA_THROW(L"No protocol specified!");

	// Look up handler in registry
	string_t strProtocol = strEndPoint.Left(pos).ToLower();

	// Make sure we are using at least one port...
	string_t strEnd = L"tcp://" + strEndPoint.Mid(pos+3);
	if (strEnd.Find(L':',pos+3) == string_t::npos)
		strEnd += L":80";

	// Create a Tcp Protocol Handler
	OTL::ObjectPtr<Omega::IO::IProtocolHandler> ptrTcp(OID_TcpProtocolHandler);

	// Create a callback handler...
	ObjectPtr<ObjectImpl<HttpCallback> > ptrCallback = ObjectImpl<HttpCallback>::CreateInstancePtr();
	

	// Create a Tcp stream
	ObjectPtr<IO::IStream> ptrStream;
	ptrStream.Attach(ptrTcp->OpenStream(strEnd,ptrCallback));

	// Init the callback
	ptrCallback->init(ptrStream,pCallback);
	
	// Return a pointer to the callback's IStream interface
	return ptrCallback.QueryInterface<IO::IStream>();
}
