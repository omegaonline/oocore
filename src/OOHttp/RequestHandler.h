///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
//
// This file is part of OOHttpd, the Omega Online HTTP Server application.
//
// OOHttpd is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOHttpd is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOHttpd.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOHTTP_REQUEST_HANDLER_H_INCLUDED_
#define OOHTTP_REQUEST_HANDLER_H_INCLUDED_

namespace Http
{
	class RequestHandler :
		public OTL::ObjectBase,
		public Omega::Net::IAsyncSocketNotify
	{
	public:
		RequestHandler() {}

		void Reset(Omega::Net::IAsyncSocket* pSocket);

		BEGIN_INTERFACE_MAP(RequestHandler)
			INTERFACE_ENTRY(Omega::Net::IAsyncSocketNotify)
		END_INTERFACE_MAP()

	private:
		OOBase::SpinLock                         m_lock;
		OTL::ObjectPtr<Omega::Net::IAsyncSocket> m_ptrSocket;

	// Net::IAsyncSocketNotify members
	public:
		void OnRecv(Omega::Net::IAsyncSocketBase* pSocket, Omega::uint32_t lenBytes, const Omega::byte_t* bytes, Omega::IException* pError);
		void OnSent(Omega::Net::IAsyncSocketBase* pSocket, Omega::uint32_t lenBytes, const Omega::byte_t* bytes, Omega::IException* pError);
		void OnClose(Omega::Net::IAsyncSocketBase* pSocket);
	};
}

#endif // OOHTTP_REQUEST_HANDLER_H_INCLUDED_
