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

using namespace Omega;
using namespace OTL;

#define DEFAULT_RECV 1400

void OOHttp::RequestHandler::Init(Server* pServer)
{
	Threading::Guard<Threading::Mutex> guard(m_lock);

	m_pServer = pServer;
}

void OOHttp::RequestHandler::Reset(Net::IAsyncSocket* pSocket)
{
	OTL::ObjectPtr<Omega::Net::IAsyncSocket> ptrOld;

	Threading::Guard<Threading::Mutex> guard(m_lock);

	if (m_ptrSocket)
	{
		ObjectPtr<Net::IAsyncSocketNotify> ptrN;
		ptrN.Attach(m_ptrSocket->Bind(0));
	}

	m_ptrSocket = pSocket;

	ObjectPtr<Net::IAsyncSocketNotify> ptrN;
	ptrN.Attach(m_ptrSocket->Bind(this));
	
	m_parseState = parseRequest;
	
	m_ptrSocket->Recv(DEFAULT_RECV,false);
}

void OOHttp::RequestHandler::OnRecv(Omega::Net::IAsyncSocketBase* /*pSocket*/, Omega::uint32_t lenBytes, const Omega::byte_t* bytes, Omega::IException* pError)
{
	if (pError)
		return;

	try
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		m_buffer.append((const char*)bytes,lenBytes);
		
		for (int res = 0;;)
		{
			switch (m_parseState)
			{
			case parseRequest:
				res = parse_request_line();
				break;

			case parseHeaders:
				res = parse_headers();
				break;

			case parseBody:
				res = parse_body();
				break;

			case recvBody:
				res = recv_body();
				break;

			case recvChunked:
				res = recv_chunked();
				break;

			case recvComplete:
				recv_complete();
				res = 1;
				break;
			}

			if (res != 0)
				return handle_result(res);
		}
	}
	catch (IException* pE)
	{
		std::string str = pE->GetDescription().ToUTF8();
		pE->Release();

		report_error(500,str.c_str());
	}
}

void OOHttp::RequestHandler::OnSent(Omega::Net::IAsyncSocketBase* /*pSocket*/, Omega::uint32_t /*lenBytes*/, const Omega::byte_t* /*bytes*/, Omega::IException* pError)
{
	if (pError)
	{
		Threading::Guard<Threading::Mutex> guard(m_lock);

		// Close ourselves on error...
		close();
	}
}

void OOHttp::RequestHandler::OnClose(Omega::Net::IAsyncSocketBase* /*pSocket*/)
{
	Threading::Guard<Threading::Mutex> guard(m_lock);

	// Close ourselves...
	close();
}

void OOHttp::RequestHandler::close()
{
	m_ptrResource = 0;
	m_ptrSocket = 0;
}

int OOHttp::RequestHandler::report_error(unsigned int status_code, const std::string& strBodyText)
{
	if (m_ptrSocket)
	{
		try
		{
			// Get the formatted error response
			std::string resp = m_pServer->GetErrorResponse(m_info.m_uVersion,status_code,strBodyText);
			m_ptrSocket->Send(resp.size(),reinterpret_cast<const byte_t*>(resp.data()));
		}
		catch (IException* pE)
		{
			pE->Release();
		}

		close();
	}

	return -1;
}

void OOHttp::RequestHandler::handle_result(int result)
{
	// The socket might have been closed along the way...
	if (!m_ptrSocket)
		return;
	
	if (result <= 0)
	{
		bool bClose = (m_info.m_uVersion == 0);
		if (!bClose)
		{
			std::map<std::string,std::string>::const_iterator i = m_info.m_mapHeaders.find("Connection");
			if (i != m_info.m_mapHeaders.end() && i->second == "close")
				bClose = true;
		}

		if (bClose)
		{
			// This will result in our Release()
			close();
			return;
		}

		m_parseState = parseRequest;
		m_buffer.clear();
	}
	else
	{
		m_buffer.cache();
	}
	
	if (result > 1)
		m_ptrSocket->Recv(result,true);
	else
		m_ptrSocket->Recv(DEFAULT_RECV,false);
}

int OOHttp::RequestHandler::parse_request_line()
{
	const char* start_ptr = m_buffer.begin();
	const char* rd_ptr = start_ptr;
	const char* end_ptr = m_buffer.end();

	// Clear the version number, it is used in report_error
	m_info.m_uVersion = 0;

	// Minimum request length = "GET / HTTP/1.X<CR><LF><CR><LF>" = 18
	if (end_ptr - rd_ptr < 18)
		return 1;
	
	// Parse method
	m_info.m_method = mUnknown;
	switch (*rd_ptr)
	{
	case 'G':
		if (rd_ptr[1] == 'E' && rd_ptr[2] == 'T')
		{
			// GET
			m_info.m_method = mGet;
			rd_ptr += 3;
		}
		break;

	case 'P':
		if (!strncmp(rd_ptr+1,"OST",3))
		{
			// POST
			m_info.m_method = mPost;
			rd_ptr += 4;
		}
		else if (!strncmp(rd_ptr+1,"UT",2))
		{
			// PUT
			m_info.m_method = mPut;
			rd_ptr += 3;
		}
		break;

	case 'O':
		if (!strncmp(rd_ptr+1,"PTIONS",6))
		{
			m_info.m_method = mOptions;
			rd_ptr += 7;
		}
		break;

	case 'H':
		if (!strncmp(rd_ptr+1,"EAD",3))
		{
			m_info.m_method = mHead;
			rd_ptr += 4;
		}
		break;

	case 'D':
		if (!strncmp(rd_ptr+1,"ELETE",5))
		{
			m_info.m_method = mDelete;
			rd_ptr += 4;
		}
		break;

	case 'T':
		if (!strncmp(rd_ptr+1,"RACE",4))
		{
			m_info.m_method = mTrace;
			rd_ptr += 5;
		}
		break;

	case 'C':
		if (!strncmp(rd_ptr+1,"ONNECT",6))
		{
			m_info.m_method = mConnect;
			rd_ptr += 7;
		}
		break;

	default:
		break;
	}

	// Check for the next space...
	if (m_info.m_method != mUnknown && *rd_ptr++ != ' ')
		m_info.m_method = mUnknown;

	if (m_info.m_method == mUnknown)
	{
		// Fast-forward to space
		while (rd_ptr < end_ptr && *rd_ptr != ' ')
			++rd_ptr;

		if (rd_ptr >= end_ptr)
			return 1;
	}
	m_info.m_strMethod.assign(start_ptr,rd_ptr-start_ptr-1);
	
	// Now pull out resource...
	const char* start = rd_ptr;
	while (rd_ptr < end_ptr && *rd_ptr != ' ')
		*rd_ptr++;

	if (rd_ptr == start)
		return report_error(400);

	m_info.m_strResource.assign(start,rd_ptr-start);
	
	// We should have 13 more characters = <SP>HTTP/1.?<CR><LF><CR><LF> minimum
	if (rd_ptr+13 >= end_ptr)
		return 1;
		
	// Check for version info
	if (strncmp(rd_ptr," HTTP/",6))
		return report_error(400);

	rd_ptr += 6;

	if (rd_ptr[0] != '1' && rd_ptr[1] != '.')
	{
		// Invalid major version
		return report_error(505);
	}

	m_info.m_uVersion = 0;
	rd_ptr += 2;
	for (int i=0;i<5;++i)
	{
		if (*rd_ptr < '0' || *rd_ptr > '9')
			break;

		m_info.m_uVersion *= 10;
		m_info.m_uVersion += (*rd_ptr++ - '0');
	}

	// Just a catch for very big minor versions
	if (m_info.m_uVersion > 0xFFFF)
		return report_error(505);

	if (rd_ptr+2 >= end_ptr)
		return 1;

	if (rd_ptr[0] != '\r' || rd_ptr[1] != '\n')
		return report_error(400);

	rd_ptr += 2;

	m_buffer.tell(rd_ptr - start_ptr);
	m_parseState = parseHeaders;

	return 0;
}

int OOHttp::RequestHandler::parse_headers()
{
	// Parse headers
	m_info.m_mapHeaders.clear();

	const char* start_ptr = m_buffer.begin();
	const char* rd_ptr = start_ptr;
	const char* end_ptr = m_buffer.end();

	for (;;)
	{
		std::string strKey,strValue;
		int res = parse_field(rd_ptr,end_ptr,strKey,strValue);
		if (res != 0)
			return res;

		if (strKey.empty())
			break;
		
		std::pair<std::map<std::string,std::string>::iterator,bool> p = m_info.m_mapHeaders.insert(std::map<std::string,std::string>::value_type(strKey,strValue));
		if (!p.second)
		{
			p.first->second.append(", ");
			p.first->second.append(strValue);
		}		
	}

	m_buffer.tell(rd_ptr - start_ptr);
	m_buffer.compact();

	m_parseState = parseBody;

	return 0;
}

bool OOHttp::RequestHandler::skip_lws(const char*& rd_ptr, const char* end_ptr)
{
	// Optional CRLF
	if (rd_ptr >= end_ptr)
		return false;
	
	if (*rd_ptr == '\r')
	{
		if (rd_ptr+1 >= end_ptr)
			return false;

		if (rd_ptr[1] != '\n')
		{
			// Not a CRL pair...
			return true;
		}
			
		// Check for SP or HT
		if (rd_ptr+2 >= end_ptr)
			return false;

		if (rd_ptr[2] != '\x20' && rd_ptr[2] != '\t')
		{
			// Not a CRLF leading WS
			return true;
		}

		rd_ptr += 2;
	}

	// Skip SP and HT
	while (rd_ptr < end_ptr && (*rd_ptr == ' ' || *rd_ptr == '\t'))
		++rd_ptr;

	return (rd_ptr < end_ptr);
}

int OOHttp::RequestHandler::parse_field(const char*& rd_ptr, const char* end_ptr, std::string& strKey, std::string& strValue)
{
	// Check for leading CRLF
	if (rd_ptr+1 >= end_ptr)
		return 1;

	if (rd_ptr[0] == '\r' && rd_ptr[1] == '\n')
	{
		strKey.clear();
		rd_ptr += 2;
		return 0;
	}

	// Fast-forward to colon
	const char* start = rd_ptr;
	while (rd_ptr < end_ptr && *rd_ptr != ':')
		++rd_ptr;

	if (rd_ptr >= end_ptr)
		return 1;

	// Stash key
	strKey.assign(start,rd_ptr-start);
	++rd_ptr;
	
	for (bool once = false;;once = true)
	{
		if (!skip_lws(rd_ptr,end_ptr))
			return 1;

		// skip_lws has a check for the buffer already
		if (rd_ptr[0] == '\r' && rd_ptr[1] == '\n')
		{
			rd_ptr += 2;
			break;
		}

		start = rd_ptr;

		while (rd_ptr < end_ptr && *rd_ptr != '\r' && *rd_ptr != ' ' && *rd_ptr != '\t')
			++rd_ptr;

		if (rd_ptr >= end_ptr)
			return 1;

		if (!once)
			strValue.assign(start,rd_ptr-start);
		else
		{
			strValue.append(1,' ');
			strValue.append(start,rd_ptr-start);
		}	
	}
				
	return 0;
}

int OOHttp::RequestHandler::parse_body()
{
	m_info.m_ulContent = 0;
	m_ulContentRead = 0;

	m_parseState = recvBody;

	if (m_info.m_uVersion >= 1)
	{
		std::map<std::string,std::string>::const_iterator i = m_info.m_mapHeaders.find("Transfer-Encoding");
		if (i != m_info.m_mapHeaders.end())
		{
			m_info.m_ulContent = size_t(-1);
			m_parseState = recvChunked;
		}
	}

	if (m_parseState == recvBody)
	{
		std::map<std::string,std::string>::const_iterator i = m_info.m_mapHeaders.find("Content-Length");
		if (i != m_info.m_mapHeaders.end())
		{
			bool bValid = false;
			const char* buf = i->second.c_str();
			for (int i=0;i<std::numeric_limits<size_t>::digits10;++i)
			{
				if (!*buf)
				{
					if (i > 0)
						bValid = true;
					break;
				}

				if (*buf < '0' || *buf > '9')
					break;

				m_ulContentRead *= 10;
				m_ulContentRead += (*buf++ - '0');
			}

			if (!bValid)
			{
				// Error
				return report_error(400);
			}

			m_info.m_ulContent = m_ulContentRead;
		}
	}

	// Ask the server for the resource...
	m_ptrResource = m_pServer->FindResource(this,m_info);
	if (!m_ptrResource)
		return report_error(400);

	return 0;
}

int OOHttp::RequestHandler::recv_body()
{
	if (m_ulContentRead)
	{
		size_t len = m_buffer.length();
		if (len > m_ulContentRead)
			len = m_ulContentRead;

		if (len > 0)
		{
			m_ulContentRead -= len;

			if (m_ptrResource)
				m_ptrResource->OnRequestBody(len,reinterpret_cast<const byte_t*>(m_buffer.begin()));

			m_buffer.tell(len);
			m_buffer.compact();
		}

		if (m_ulContentRead)
			return m_ulContentRead;
	}
	
	m_parseState = recvComplete;
	return 0;
}

int OOHttp::RequestHandler::recv_chunked()
{
	//const char* start_ptr = buffer->rd_ptr();
	//const char* rd_ptr = start_ptr;
	//const char* end_ptr = buffer->wr_ptr();

	for (;;)
	{
		// Hex

		void* TODO;
	}
	
	m_parseState = recvComplete;
	return 0;
}

void OOHttp::RequestHandler::recv_complete()
{
	if (m_ptrResource)
	{
		m_ptrResource->OnRequestComplete();
		m_ptrResource = 0;
	}
}

void OOHttp::RequestHandler::Send(uint32_t lenBytes, const byte_t* data)
{
	Threading::Guard<Threading::Mutex> guard(m_lock);

	if (m_ptrSocket)
		m_ptrSocket->Send(lenBytes,data);
}

void OOHttp::RequestHandler::SendHeader(uint16_t status_code, const Http::Server::IRequest::header_map_t& mapHeaders)
{
	std::string strText,strExtended;
	GetStatusText(status_code,m_info.m_uVersion,strText,strExtended);

	std::ostringstream out_stream;
	out_stream.imbue(std::locale::classic());

	if (m_info.m_uVersion >= 1)
		out_stream << "HTTP/1.1 " << strText << "\r\n";
	else
		out_stream << "HTTP/1.0 " << strText << "\r\n";

	std::map<std::string,std::string> headers;
	m_pServer->GetHeaders(headers);

	// Add the new items, without overwriting any server variables
	for (Http::Server::IRequest::header_map_t::const_iterator i=mapHeaders.begin();i!=mapHeaders.end();++i)
		headers.insert(std::map<std::string,std::string>::value_type(i->first.ToUTF8(),i->second.ToUTF8()));
		
	for (std::map<std::string,std::string>::const_iterator i=headers.begin();i!=headers.end();++i)
		out_stream << i->first << ": " << i->second << "\r\n";

	out_stream << "\r\n";
	std::string str = out_stream.str();

	m_ptrSocket->Send(str.size(),reinterpret_cast<const byte_t*>(str.data()));
}
