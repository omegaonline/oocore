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

#ifndef OOHTTP_REQUEST_H_INCLUDED_
#define OOHTTP_REQUEST_H_INCLUDED_

#include "RequestHandler.h"

namespace OOHttp
{
	class Request : 
			public OTL::ObjectBase,
			public Omega::Http::Server::IRequest
	{
	public:
		void Init(IRequestHandler* pHandler, const RequestHandler::Info& info);

		BEGIN_INTERFACE_MAP(Request)
			INTERFACE_ENTRY(Omega::Http::Server::IRequest)
		END_INTERFACE_MAP()

	private:
		OTL::ObjectPtr<IRequestHandler> m_ptrHandler;
		Omega::uint16_t                 m_uVersion;
		Omega::string_t                 m_strMethod;
		Omega::string_t                 m_strHost;
		Omega::string_t                 m_strResource;
		header_map_t                    m_mapHeaders;
		Omega::uint32_t                 m_ulContent;

	// IRequest members
	public:
		virtual Omega::uint16_t GetHTTPVersion();
		virtual	Omega::string_t GetMethod();
		virtual Omega::string_t GetHost();
		virtual Omega::string_t GetResource();
		virtual header_map_t GetHeaders();
		virtual Omega::uint32_t GetContentLength();
		virtual void SendResponseHeader(Omega::uint16_t status_code, const header_map_t& mapHeaders);
		virtual void SendResponseBody(Omega::uint32_t lenBytes, const Omega::byte_t* bytes);
	};
}

#endif // OOHTTP_REQUEST_H_INCLUDED_
