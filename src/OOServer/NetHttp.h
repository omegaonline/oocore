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

#ifndef OOSERVER_NETHTTP_H_INCLUDED_
#define OOSERVER_NETHTTP_H_INCLUDED_

namespace User
{
	// {EDB0676F-70B0-4e49-AACC-E8478F615277}
	OMEGA_EXPORT_OID(OID_HttpProtocolHandler);

	class HttpProtocolHandler :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactoryNoAggregation<HttpProtocolHandler,&OID_HttpProtocolHandler,Omega::Activation::InProcess>,
		public Omega::IO::IProtocolHandler
	{
	public:
		BEGIN_INTERFACE_MAP(HttpProtocolHandler)
			INTERFACE_ENTRY(Omega::IO::IProtocolHandler)
		END_INTERFACE_MAP()

	private:
		Omega::string_t FindProxy(const Omega::string_t& strURL, const Omega::string_t& strProtocol);

	// IProtocolHandler members
	public:
		Omega::IO::IStream* OpenStream(const Omega::string_t& strEndPoint, Omega::IO::IAsyncStreamCallback* pCallback);
	};
}

#endif // OOSERVER_NETHTTP_H_INCLUDED_
