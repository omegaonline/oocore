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
		public IO::IStream,
		public ACE_Service_Handler
	{
	public:
		TcpStream();

		void init(ACE_HANDLE hStream);
		
		BEGIN_INTERFACE_MAP(TcpStream)
			INTERFACE_ENTRY(IO::IStream)
		END_INTERFACE_MAP()

	private:
		
	// IStream members
	public:
		void ReadBytes(uint32_t& cbBytes, byte_t* val);
		uint32_t WriteBytes(uint32_t cbBytes, const byte_t* val);
	};
}

User::TcpStream::TcpStream()
{
}

void User::TcpStream::init(ACE_HANDLE hStream)
{
}

void User::TcpStream::ReadBytes(uint32_t& cbBytes, byte_t* val)
{
}

uint32_t User::TcpStream::WriteBytes(uint32_t cbBytes, const byte_t* val)
{
	return 0;
}

IO::IStream* User::TcpProtocolHandler::OpenStream(const string_t& strEndPoint, IO::IAsyncStreamCallback* pCallback)
{
	// First try to determine the protocol...
	size_t pos = strEndPoint.Find(L"://");
	if (pos == string_t::npos)
		OMEGA_THROW(L"No protocol specified!");

	ACE_INET_Addr addr(ACE_TEXT_WCHAR_TO_TCHAR(strEndPoint.Mid(pos+3).c_str()));

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

	if (deadline != ACE_Time_Value::max_time)
	{
		ACE_Time_Value now = ACE_OS::gettimeofday();
		if (deadline <= now)
			OMEGA_THROW(ETIMEDOUT);

		deadline -= now;
	}

	ACE_SOCK_Stream stream;
	if (ACE_SOCK_Connector().connect(stream,addr,deadline == ACE_Time_Value::max_time ? 0 : &deadline) != 0)
		OMEGA_THROW(ACE_OS::last_error());

	ObjectPtr<ObjectImpl<TcpStream> > ptrStream = ObjectImpl<TcpStream>::CreateInstancePtr();
	ptrStream->init(stream.get_handle());
	stream.set_handle(ACE_INVALID_HANDLE);

	return 0;
}
