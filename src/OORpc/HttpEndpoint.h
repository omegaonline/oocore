///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OORpc, the Omega Online RPC library.
//
// OORpc is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OORpc is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OORpc.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OORPC_HTTP_ENDPOINT_H_INCLUDED_
#define OORPC_HTTP_ENDPOINT_H_INCLUDED_

namespace Rpc
{
	// {62CF3425-D60F-4f3e-920B-6CAFDD8AAA77}
	OMEGA_EXPORT_OID(OID_HttpEndpoint);

	class HttpEndpoint :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactorySingleton<HttpEndpoint,&OID_HttpEndpoint,0,Omega::Activation::InProcess>,
		public Omega::Remoting::IEndpoint
	{
	public:
		virtual ~HttpEndpoint()
		{
		}

		BEGIN_INTERFACE_MAP(HttpEndpoint)
			INTERFACE_ENTRY(Omega::Remoting::IEndpoint)
		END_INTERFACE_MAP()

		static void install(Omega::bool_t bInstall, const Omega::string_t& strSubsts);

	// IEndpoint members
	public:
		Omega::string_t Canonicalise(const Omega::string_t& strEndpoint);
		Omega::guid_t MessageOid();
		Omega::Remoting::IChannelSink* Open(const Omega::string_t& strEndpoint, Omega::Remoting::IChannelSink* pSink);
	};
}

#endif // OORPC_HTTP_ENDPOINT_H_INCLUDED_
