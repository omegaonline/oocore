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

#ifndef OOHTTP_SERVER_BITS_H_INCLUDED_
#define OOHTTP_SERVER_BITS_H_INCLUDED_

#include "../../include/Omega/Omega.h"

namespace Omega
{
	namespace Http
	{
		namespace Server
		{
			interface IResource : public IObject
			{
				virtual void OnRequestBody(uint32_t lenBytes, const byte_t* bytes) = 0;
				virtual void OnRequestComplete() = 0;
			};

			interface IRequest : public IObject
			{
				typedef std::map<string_t,string_t> header_map_t;

				virtual uint16_t GetHTTPVersion() = 0;
				virtual	string_t GetMethod() = 0;
				virtual string_t GetHost() = 0;
				virtual string_t GetResource() = 0;
				virtual header_map_t GetHeaders() = 0;
				virtual uint32_t GetContentLength() = 0;
				virtual void SendResponseHeader(uint16_t status_code, const header_map_t& mapHeaders = header_map_t()) = 0;
				virtual void SendResponseBody(uint32_t lenBytes, const byte_t* bytes) = 0;
			};

			interface IResourceHandler : public IObject
			{
				virtual IResource* GetResource(IRequest* pRequest) = 0;
			};
		}
	}
}

OMEGA_DEFINE_INTERFACE
(
	Omega::Http::Server, IResource, "{5DC59CE2-27F3-4505-A117-142B8D29AE6C}",

	OMEGA_METHOD_VOID(OnRequestBody,2,((in),uint32_t,lenBytes,(in)(size_is(lenBytes)),const byte_t*,bytes))
	OMEGA_METHOD_VOID(OnRequestComplete,0,())
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Http::Server, IRequest, "{55C9523C-C0CD-44A4-B564-10E9D9EBF5BD}",

	OMEGA_METHOD(uint16_t,GetHTTPVersion,0,())
	OMEGA_METHOD(string_t,GetMethod,0,())
	OMEGA_METHOD(string_t,GetHost,0,())
	OMEGA_METHOD(string_t,GetResource,0,())
	OMEGA_METHOD(Http::Server::IRequest::header_map_t,GetHeaders,0,())
	OMEGA_METHOD(uint32_t,GetContentLength,0,())
	OMEGA_METHOD_VOID(SendResponseHeader,2,((in),uint16_t,status_code,(in),const Http::Server::IRequest::header_map_t&,mapHeaders))
	OMEGA_METHOD_VOID(SendResponseBody,2,((in),uint32_t,lenBytes,(in)(size_is(lenBytes)),const byte_t*,bytes))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Http::Server, IResourceHandler, "{0273A32C-B305-43AD-816A-C7823CF84FE8}",

	OMEGA_METHOD(Http::Server::IResource*,GetResource,1,((in),Http::Server::IRequest*,pRequest))
)


#endif // OOHTTP_SERVER_BITS_H_INCLUDED_
