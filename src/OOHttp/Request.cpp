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

#include "OOHttpd.h"
#include "HttpServer.h"
#include "Request.h"

using namespace Omega;
using namespace OTL;

void OOHttp::Request::Init(IRequestHandler* pHandler, const RequestHandler::Info& info)
{
	m_ptrHandler = pHandler;

	m_uVersion = static_cast<uint16_t>(info.m_uVersion);
	m_strMethod = string_t(info.m_strMethod.c_str(),true,info.m_strMethod.length());
	m_strHost = string_t(info.m_strHost.c_str(),true,info.m_strHost.length());
	m_strResource = string_t(info.m_strResource.c_str(),true,info.m_strResource.length());
	m_ulContent = info.m_ulContent;

	for (std::map<std::string,std::string>::const_iterator i=info.m_mapHeaders.begin();i!=info.m_mapHeaders.end();++i)
		m_mapHeaders.insert(header_map_t::value_type(string_t(i->first.c_str(),true,i->first.length()),string_t(i->second.c_str(),true,i->second.length())));
}

uint16_t OOHttp::Request::GetHTTPVersion()
{
	return m_uVersion;
}

string_t OOHttp::Request::GetMethod()
{
	return m_strMethod;
}

string_t OOHttp::Request::GetHost()
{
	return m_strHost;
}

string_t OOHttp::Request::GetResource()
{
	return m_strResource;
}

Http::Server::IRequest::header_map_t OOHttp::Request::GetHeaders()
{
	return m_mapHeaders;
}

uint32_t OOHttp::Request::GetContentLength()
{
	return m_ulContent;
}

void OOHttp::Request::SendResponseHeader(uint16_t status_code, const header_map_t& mapHeaders)
{
	m_ptrHandler->SendHeader(status_code,mapHeaders);
}

void OOHttp::Request::SendResponseBody(uint32_t lenBytes, const byte_t* bytes)
{
	m_ptrHandler->Send(lenBytes,bytes);
}
