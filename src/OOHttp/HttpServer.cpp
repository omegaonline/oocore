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

namespace
{
	class NullResource : 
			public ObjectBase,
			public Http::Server::IResource
	{
	public:
		void Init(uint16_t status_code, Http::Server::IRequest* pRequest)
		{
			m_status_code = status_code;
			m_ptrRequest = pRequest;
		}

		BEGIN_INTERFACE_MAP(NullResource)
			INTERFACE_ENTRY(Http::Server::IResource)
		END_INTERFACE_MAP()

	private:
		uint16_t                          m_status_code;
		ObjectPtr<Http::Server::IRequest> m_ptrRequest;

	// IResource members
	public:
		void OnRequestBody(uint32_t, const byte_t*)
		{}
		
		void OnRequestComplete()
		{
			Http::Server::IRequest::header_map_t mapHeaders;

			if (m_ptrRequest->GetHTTPVersion() >= 1)
				mapHeaders.insert(std::map<string_t,string_t>::value_type(L"Cache-Control",L"no-cache"));
			else
				mapHeaders.insert(std::map<string_t,string_t>::value_type(L"Pragma",L"no-cache"));

			/*std::string strText,strExtended;
			OOHttp::GetStatusText(m_status_code,m_ptrRequest->GetHTTPVersion(),strText,strExtended);

			if (!strExtended.empty())
			{			
				std::ostringstream out_stream;
				out_stream.imbue(std::locale::classic());
				out_stream << strExtended.size();
				mapHeaders.insert(std::map<string_t,string_t>::value_type(L"Content-Length",string_t(out_stream.str().c_str(),true)));
				mapHeaders.insert(std::map<string_t,string_t>::value_type(L"Content-Type",L"text/plain; charset=UTF-8"));
			}

			m_ptrRequest->SendResponseHeader(m_status_code,mapHeaders);

			if (!strExtended.empty())
				m_ptrRequest->SendResponseBody(strExtended.size(),reinterpret_cast<const byte_t*>(strExtended.data()));
				*/

			std::string strText = "Hello world from the OOHttp server!\n\nOne day all things will be made of this stuff";

			std::ostringstream out_stream;
			out_stream.imbue(std::locale::classic());
			out_stream << strText.size();

			mapHeaders.insert(std::map<string_t,string_t>::value_type(L"Content-Length",string_t(out_stream.str().c_str(),true)));
			mapHeaders.insert(std::map<string_t,string_t>::value_type(L"Content-Type",L"text/plain; charset=UTF-8"));

			m_ptrRequest->SendResponseHeader(200,mapHeaders);
			m_ptrRequest->SendResponseBody(strText.size(),reinterpret_cast<const byte_t*>(strText.data()));
		}
	};
}

void OOHttp::Server::InitOnce()
{
}

void OOHttp::Server::SetRegistryKey(Registry::IKey* pKey)
{
	Threading::Guard<Threading::Mutex> guard(m_lock);

	if (!m_ptrKey)
		m_ptrKey = pKey;
}

void OOHttp::Server::OnAccept(Net::IAsyncSocket* pSocket)
{
	if (!pSocket)
		return;

	// Now to connect up a new request handler...
	ObjectPtr<ObjectImpl<RequestHandler> > ptrHandler = ObjectImpl<RequestHandler>::CreateInstancePtr();
	ptrHandler->Init(this);
	ptrHandler->Reset(pSocket);
}

std::string OOHttp::Server::GetVersion() const
{
	return "OOHttp/0.1";
}

void OOHttp::Server::GetHeaders(std::map<std::string,std::string>& headers)
{
	time_t t = time(0);

#if defined(_MSC_VER)
	::tm tm_now = {0};
	gmtime_s(&tm_now,&t);
#else
	::tm tm_now = *gmtime(&t);
#endif

	static const char* days[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static const char* months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	std::ostringstream out_stream;
	out_stream.imbue(std::locale::classic());

	out_stream.fill('0');
	out_stream << days[tm_now.tm_wday] << ", ";
	out_stream << std::setw(2) << tm_now.tm_mday << " ";
	out_stream << months[tm_now.tm_mon] << " ";
	out_stream << std::setw(4) << (tm_now.tm_year + 1900) << " ";
	out_stream << std::setw(2) << tm_now.tm_hour << ":";
	out_stream << std::setw(2) << tm_now.tm_min << ":";
	out_stream << std::setw(2) << tm_now.tm_sec << " GMT";

	headers.insert(std::map<std::string,std::string>::value_type("Date",out_stream.str()));
	headers.insert(std::map<std::string,std::string>::value_type("Server",GetVersion()));
}

std::string OOHttp::Server::GetErrorResponse(unsigned long nVersion, unsigned int status_code, const std::string strBodyText, const std::map<std::string,std::string>& headers)
{
	std::string strText,strExtended;
	GetStatusText(status_code,nVersion,strText,strExtended);
	
	std::map<std::string,std::string> real_headers;
	GetHeaders(real_headers);

	if (nVersion >= 1)
		real_headers.insert(std::map<std::string,std::string>::value_type("Cache-Control","no-cache"));
	else
		real_headers.insert(std::map<std::string,std::string>::value_type("Pragma","no-cache"));
	
	if (!strBodyText.empty())
		strExtended = strBodyText;
	
	std::ostringstream out_stream;
	out_stream.imbue(std::locale::classic());

	if (!strExtended.empty())
	{
		out_stream << strExtended.size();
		real_headers.insert(std::map<std::string,std::string>::value_type("Content-Length",out_stream.str()));
		out_stream.str(std::string());
		real_headers.insert(std::map<std::string,std::string>::value_type("Content-Type","text/plain; charset=UTF-8"));
	}
	
	// Copy incoming headers
	real_headers.insert(headers.begin(),headers.end());

	if (nVersion >= 1)
		out_stream << "HTTP/1.1 " << strText << "\r\n";
	else
		out_stream << "HTTP/1.0 " << strText << "\r\n";
	
	for (std::map<std::string,std::string>::const_iterator i=real_headers.begin();i!=real_headers.end();++i)
		out_stream << i->first << ": " << i->second << "\r\n";

	out_stream << "\r\n";

	if (!strExtended.empty())
		out_stream << strExtended;
	
	return out_stream.str();
}

ObjectPtr<Http::Server::IResource> OOHttp::Server::FindResource(IRequestHandler* request, RequestHandler::Info& info)
{
	// Work out what we have in terms of a resource...
	if (info.m_method == RequestHandler::mOptions && info.m_strResource == "*")
	{
		return options_resource(request,info);
	}
	/*else if (info.m_method == RequestHandler::mTrace || info.m_method == RequestHandler::mConnect)
	{
		std::map<std::string,std::string> headers;
		headers.insert(std::map<std::string,std::string>::value_type("Allow","GET, HEAD, POST, PUT, DELETE, OPTIONS"));
		ReportError(request,info.m_nVersion,405,headers);
	}*/
	
	if (info.m_strResource[0] == '/')
	{
		info.m_strHost = "localhost";

		if (info.m_uVersion >= 1)
		{
			std::map<std::string,std::string>::const_iterator i=info.m_mapHeaders.find("Host");
			if (i != info.m_mapHeaders.end())
				info.m_strHost = i->second;
		}

		return find_resource(request,info);
	}
	
	if (info.m_strResource.length() >= 7)
	{
		static const char* http_ucase = "HTTP://";
		static const char* http_lcase = "http://";

		size_t pos = 0;
		for (;pos<7;++pos)
		{
			if (info.m_strResource[pos] != http_lcase[pos] && info.m_strResource[pos] != http_ucase[pos])
				break;
		}

		if (pos == 7)
		{
			size_t slash = info.m_strResource.find('/',pos);
			if (slash == std::string::npos)
			{
				info.m_strHost = info.m_strResource.substr(7);
				info.m_strResource = "/";
			}
			else
			{
				info.m_strHost = info.m_strResource.substr(7,slash);
				info.m_strResource = info.m_strResource.substr(slash);
			}

			return find_resource(request,info);
		}
	}

	// We have a 400 error... return NULL
	return ObjectPtr<Http::Server::IResource>();
}

ObjectPtr<Http::Server::IResource> OOHttp::Server::options_resource(IRequestHandler* request, const RequestHandler::Info& info)
{
	// Create a new IRequest
	ObjectPtr<ObjectImpl<Request> > ptrRequest = ObjectImpl<Request>::CreateInstancePtr();
	ptrRequest->Init(request,info);

	void* OPTIONS_REQUEST;

	return ObjectPtr<Http::Server::IResource>();
}

ObjectPtr<Http::Server::IResource> OOHttp::Server::find_resource(IRequestHandler* request, const RequestHandler::Info& info)
{
	// Create a new IRequest
	ObjectPtr<ObjectImpl<Request> > ptrRequest = ObjectImpl<Request>::CreateInstancePtr();
	ptrRequest->Init(request,info);

	ObjectPtr<ObjectImpl<NullResource> > ptrRes = ObjectImpl<NullResource>::CreateInstancePtr();
	ptrRes->Init(404,ptrRequest);

	return ptrRes;
}

void OOHttp::GetStatusText(unsigned int status_code, unsigned long nVersion, std::string& strText, std::string& strExtended)
{
	switch (status_code)
	{
	case 200:
		strText = "200 OK";
		break;

	case 201:
		strText = "201 Created";
		break;

	case 202:
		strText = "202 Accepted";
		break;

	case 204:
		strText = "204 No Content";
		break;

	case 301:
		strText = "301 Moved Permanently";
		break;

	case 302:
		if (nVersion == 0)
			strText = "302 Moved Temporarily";
		else
			strText = "302 Found";
		break;

	case 304:
		strText = "304 Not Modified";
		break;

	case 400:
		strText = "400 Bad Request";
		strExtended = "The request could not be understood by the server due to malformed syntax.";
		break;

	case 401:
		strText = "401 Unauthorized";
		strExtended = "The request requires user authentication.";
		break;

	case 403:
		strText = "403 Forbidden";
		strExtended = "The server understood the request, but is refusing to fulfill it. Authorization will not help and the request SHOULD NOT be repeated.";
		break;

	case 404:
		strText = "404 Not Found";
		strExtended = "The server has not found anything matching the Request-URI.";
		break;

	case 500:
		strText = "500 Internal Server Error";
		strExtended = "The server encountered an unexpected condition which prevented it from fulfilling the request.";
		break;

	case 501:
		strText = "501 Not Implemented";
		strExtended = "The server does not support the functionality required to fulfill the request.";
		break;

	case 502:
		strText = "502 Bad Gateway";
		strExtended = "The server, while acting as a gateway or proxy, received an invalid response from the upstream server it accessed in attempting to fulfill the request.";
		break;

	case 503:
		strText = "503 Service Unavailable";
		strExtended = "The server is currently unable to handle the request due to a temporary overloading or maintenance of the server.";
		break;

	default:
		break;
	}

	if (nVersion >= 1 && strText.empty())
	{
		switch (status_code)
		{
		case 405:
			strText = "405 Method Not Allowed";
			strExtended = "The method specified in the Request-Line is not allowed for the resource identified by the Request-URI.";
			break;

		case 411:
			strText = "411 Length Required";
			strExtended = "The server refuses to accept the request without a defined Content-Length. The client MAY repeat the request if it adds a valid Content-Length header field containing the length of the message-body in the request message.";
			break;

		case 505:
			strText = "505 HTTP Version Not Supported";
			strExtended = "The server does not support, or refuses to support, the HTTP protocol version that was used in the request message.\r\nPlease use HTTP/1.0 or HTTP/1.1";
			break;

		default:
			assert(false);
			break;
		}
	}

	if (strText.empty())
	{
		strText = "400 Bad Request";
		strExtended = "The request could not be understood by the server due to malformed syntax.";
	}
}
