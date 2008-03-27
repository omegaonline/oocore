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

#ifndef OOSERVER_NETTCP_H_INCLUDED_
#define OOSERVER_NETTCP_H_INCLUDED_

namespace User
{
	// {4924E463-06A4-483b-9DAD-8BFD83ADCBFC}
	OMEGA_EXPORT_OID(OID_TcpProtocolHandler);

	class TcpProtocolHandler :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactoryNoAggregation<TcpProtocolHandler,&OID_TcpProtocolHandler,Omega::Activation::InProcess>,
		public Omega::IO::IProtocolHandler
	{
	public:
		BEGIN_INTERFACE_MAP(TcpProtocolHandler)
			INTERFACE_ENTRY(Omega::IO::IProtocolHandler)
		END_INTERFACE_MAP()

	// IProtocolHandler members
	public:
		Omega::IO::IStream* OpenStream(const Omega::string_t& strEndPoint, Omega::IO::IAsyncStreamCallback* pCallback);
	};
}

#endif // OOSERVER_NETTCP_H_INCLUDED_
