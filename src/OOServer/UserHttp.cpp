///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
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

#include "./OOServer_User.h"
#include "./UserManager.h"
#include "../Common/Version.h"

using namespace Omega;
using namespace OTL;

namespace User
{
	class HttpRequest :
		public ObjectBase,
		public Net::Http::Server::IRequest
	{
	public:
		HttpRequest();
		virtual ~HttpRequest();
		void init(HttpConnection* pConn, const string_t& strResource);

		BEGIN_INTERFACE_MAP(HttpRequest)
			INTERFACE_ENTRY(Net::Http::Server::IRequest)
		END_INTERFACE_MAP()

	private:
		HttpConnection*    m_pConn;
		string_t           m_strMethod;
		string_t           m_strResource;
		ACE_Message_Block* m_mbRecv;

		std::map<string_t,HttpConnection::Header> m_mapRequestHeaders;

	// IRequest members
	public:
		string_t Method();
		string_t Resource();
		Omega::IEnumString* GetAllRequestHeaders();
		string_t GetRequestHeader(const string_t& strHeader);
		void RequestBody(uint32_t& cbBytes, byte_t* pBody);
		IO::IStream* RequestStream();
	};

	class ResponseStream :
		public ObjectBase,
		public IO::IStream
	{
	public:
		ResponseStream();
		virtual ~ResponseStream();

		void init(Manager* pManager, uint16_t conn_id, bool bClose, unsigned long length);

		BEGIN_INTERFACE_MAP(ResponseStream)
			INTERFACE_ENTRY(IO::IStream)
		END_INTERFACE_MAP()

	private:
		Manager* m_pManager;
		uint16_t m_conn_id;
		bool     m_bClose;
		bool     m_bHasLength;

		ACE_Atomic_Op<ACE_Thread_Mutex,unsigned long> m_content_length;

	public:
		void ReadBytes(uint64_t& cbBytes, byte_t* val);
		void WriteBytes(const uint64_t& cbBytes, const byte_t* val);
	};

	class HttpResponse :
		public ObjectBase,
		public Net::Http::Server::IResponse
	{
	public:
		HttpResponse();
		void init(Manager* pManager, uint16_t conn_id);

		BEGIN_INTERFACE_MAP(HttpResponse)
			INTERFACE_ENTRY(Net::Http::Server::IResponse)
		END_INTERFACE_MAP()

	private:
		ACE_Thread_Mutex m_lock;
		Manager*         m_pManager;
		uint16_t         m_conn_id;

		std::map<string_t,HttpConnection::Header> m_mapResponseHeaders;

	// IResponse members
	public:
		void SetResponseHeader(const string_t& strHeader, const string_t& strValue);
		IO::IStream* Send(uint16_t uStatus, const string_t& strStatusText);
	};
}

User::HttpConnection::HttpConnection(Manager* pManager, uint16_t conn_id, const ACE_CString& strRemoteAddr, const ACE_CString& strScheme) :
	m_strScheme(strScheme),
	m_mbRecv(0),
	m_seq_no(0),
	m_pManager(pManager),
	m_conn_id(conn_id),
	m_strRemoteAddr(strRemoteAddr),
	m_read_state(request_header)
{
}

User::HttpConnection::~HttpConnection()
{
	if (m_mbRecv)
		m_mbRecv->release();
}

string_t User::HttpConnection::BaseURI()
{
	return string_t(m_strScheme.c_str(),false) + GetRequestHeader(L"Host").ToLower();
}

std::string User::HttpConnection::Trim(const std::string& str)
{
	size_t start = str.find_first_not_of(' ');
	if (start == std::string::npos)
		start = 0;

	size_t end = str.find_last_not_of(' ');
	return str.substr(start,end+1);
}

const char* User::HttpConnection::QuickFind(ACE_Message_Block* mb, const char* pStart, char c)
{
	const char* pEnd = pStart;
	while (*pEnd != c && pEnd < mb->wr_ptr())
		++pEnd;

	return (*pEnd == c ? pEnd : 0);
}

void User::HttpConnection::SplitHTTPHeader()
{
	m_mapRequestHeaders.clear();

	for (;;)
	{
		const char* pStart = m_mbRecv->rd_ptr();
		const char* pEnd = QuickFind(m_mbRecv,pStart,'\r');
		if (pEnd)
			m_mbRecv->rd_ptr(pEnd - pStart + 2);
		else
		{
			pEnd = m_mbRecv->wr_ptr();
			m_mbRecv->rd_ptr(pEnd - pStart);
		}

		std::string str(pStart,pEnd - pStart);
		if (str.empty())
			break;

		Header h;
		size_t pos = str.find(':');
		h.strHeader = string_t(Trim(str.substr(0,pos)).c_str(),false);

		if (pos < str.size()-1)
			h.strValue = string_t(Trim(str.substr(pos+1)).c_str(),false);

		m_mapRequestHeaders[h.strHeader.ToLower()] = h;
	}
}

string_t User::HttpConnection::GetRequestHeader(const string_t& strHeader)
{
	std::map<string_t,Header>::iterator i = m_mapRequestHeaders.find(strHeader.ToLower());
	if (i == m_mapRequestHeaders.end())
		return string_t();

	return i->second.strValue;
}

bool User::HttpConnection::ReadRequestHeader(const ACE_Message_Block* mb)
{
	// Append current data
	if (!m_mbRecv)
	{
		m_mbRecv = mb->duplicate();
	}
	else
	{
		// Enlarge the buffer to fit
		if (m_mbRecv->space() < mb->length())
			m_mbRecv->size(m_mbRecv->size() + mb->length());

		// Copy the extra
		m_mbRecv->copy(mb->rd_ptr(),mb->length());
	}

	// Split up the request line into parts
	const char* pStart = m_mbRecv->rd_ptr();
	const char* pCrLf = QuickFind(m_mbRecv,pStart,'\r');
	if (!pCrLf)
	{
		if (m_mbRecv->length() > 1024)
		{
			// Should have had something by now!
			HTTPRespondError("400 Bad Request");
		}

		// Wait for more...
		return false;
	}
	if (pCrLf[1] != '\n')
	{
		HTTPRespondError("400 Bad Request");
		return false;
	}

	// Check the request line
	std::string strRequestLine(pStart,pCrLf - pStart);
	strRequestLine = Trim(strRequestLine);

	size_t pos = strRequestLine.find(' ');
	if (pos == std::string::npos)
	{
		HTTPRespondError("400 Bad Request");
		return false;
	}

	m_strMethod = string_t(strRequestLine.substr(0,pos).c_str(),true);
	strRequestLine = Trim(strRequestLine.substr(pos));

	pos = strRequestLine.find(' ');
	if (pos == std::string::npos)
	{
		HTTPRespondError("400 Bad Request");
		return false;
	}

	m_strResource = string_t(strRequestLine.substr(0,pos).c_str(),true);
	if (m_strResource[0] != L'/')
	{
		// Parse the request out to its bits...
		string_t strScheme, strHost, strPort, strUserName, strPassword, strResource, strQuery;
		Net::Http::SplitURL(m_strResource,strScheme,strHost,strPort,strUserName,strPassword,strResource,strQuery);

		m_strResource = strResource;
	}

	strRequestLine = Trim(strRequestLine.substr(pos));

	if (strRequestLine.substr(0,5) != "HTTP/")
	{
		HTTPRespondError("400 Bad Request");
		return false;
	}

	if (strRequestLine != "HTTP/1.1")
	{
		HTTPRespondError("505 HTTP Version Not Supported");
		return false;
	}

	// Now check we have all the request headers
	for (;;)
	{
		const char* pEnd = QuickFind(m_mbRecv,pStart,'\r');
		if (!pEnd)
		{
			// We definitely need more
			return false;
		}

		if (pEnd - pStart == 0)
		{
			// We have got everything we need
			break;
		}

		// Start at next line...
		pStart = pEnd + 2;
	}

	// mb now contains a full response
	SplitHTTPHeader();

	// HTTP/1.1 requires a Host header
	if (GetRequestHeader(L"Host").IsEmpty())
	{
		HTTPRespondError("400 Bad Request");
		return false;
	}

	// Try to work out whether we are chunked or single block
	m_content_length = 0;
	m_read_state = request_data;
	string_t strC = GetRequestHeader(L"Content-Length");
	if (!strC.IsEmpty())
		m_content_length = ACE_OS::strtoul(strC.c_str(),0,10);
	else
	{
		strC = GetRequestHeader(L"Transfer-Encoding");
		if (!strC.IsEmpty())
		{
			if (strC != L"chunked")
			{
				HTTPRespondError("411 Length Required");
				return false;
			}

			m_read_state = request_chunked;
		}
	}

	// Do some last minute validation...
	if (m_strMethod == L"OPTIONS")
	{
		if (m_content_length != 0 || m_read_state == request_chunked)
		{
			// Options with a body must have a content type
			if (GetRequestHeader(L"Content-Type").IsEmpty())
			{
				HTTPRespondError("400 Bad Request");
				return false;
			}
		}
	}
	if (m_strMethod == L"TRACE")
	{
		if (m_content_length != 0 || m_read_state == request_chunked || m_mbRecv->length() > 0)
		{
			HTTPRespondError("400 Bad Request");
			return false;
		}
	}

	if (m_mbRecv->length() == 0)
	{
		// Check for 100 Continue
		strC = GetRequestHeader(L"Expect");
		if (strC == L"100-continue")
		{
			// We always say "Carry on..."
			ACE_CString strHeader =
				"HTTP/1.1 100 Continue\r\n"
				"Server: OOServer/" OOCORE_VERSION "\r\n"
				"Content-Length: 0\r\n"
				"\r\n";

			ACE_Message_Block mb(strHeader.length());
			mb.copy(strHeader.c_str(),strHeader.length());
			SendToRoot(&mb);
		}
	}

	return true;
}

void User::HttpConnection::ReadRequestData(const ACE_Message_Block* mb)
{
	if (mb)
	{
		// Skip to last block
		ACE_Message_Block* mb2;
		for (mb2 = m_mbRecv;mb2->cont();mb2=mb2->cont())
		{}

		// Append to chain
		mb2->cont(mb->duplicate());
	}

	// Single linear block... work out what's left to read
	if (m_content_length == m_mbRecv->total_length())
	{
		// Back to a ready state
		m_read_state = request_header;

		// Let the manager handle it
		m_pManager->handle_http_request(this,m_conn_id);

		if (m_mbRecv->cont())
		{
			m_mbRecv->cont()->release();
			m_mbRecv->cont(0);
		}
		m_mbRecv->reset();
	}
}

void User::HttpConnection::ReadRequestChunked(const ACE_Message_Block* mb_new)
{
	// Skip to last block
	ACE_Message_Block* mb;
	for (mb = m_mbRecv;mb->cont();mb=mb->cont())
	{}

	if (mb_new)
	{
		// Enlarge the buffer to fit
		if (mb->space() < mb_new->length())
			mb->size(mb->size() + mb_new->length());

		// Copy the extra
		mb->copy(mb_new->rd_ptr(),mb_new->length());
	}

	// Try to parse the chunking...
	for (;;)
	{
		// Find the first \r\n
		const char* pStart = mb->rd_ptr();
		const char* pEnd = QuickFind(mb,pStart,'\r');
		if (!pEnd)
		{
			if (mb->space() < 512)
			{
				// Create a new buffer
				ACE_Message_Block* pNewBuffer = 0;
				OMEGA_NEW(pNewBuffer,ACE_Message_Block(ACE_OS::getpagesize()));
				mb->cont(pNewBuffer);
				mb = pNewBuffer;
			}
			return;
		}

		// Find the chunk size bytes
		std::string strLen;
		const char* pExt = QuickFind(mb,pStart,';');
		if (pExt)
			strLen = std::string(pStart,pExt-pStart);
		else
			strLen = std::string(pStart,pEnd-pStart);
		pEnd += 2;

		// Parse the chunk size
		uint32_t chunk_len = ACE_OS::strtoul(strLen.c_str(),NULL,16);

		// Check we have a whole chunk
		size_t len = mb->length();
		size_t curr_len = (pEnd - pStart);
		if (len - curr_len < chunk_len + 2)
		{
			// No, we don't...
			return;
		}

		// Move pStart to the next chunk start
		pStart = pEnd + chunk_len;
		if (pStart[0] != '\r' || pStart[1] != '\n')
		{
			HTTPRespondError("400 Bad Request");
			return;
		}
		pStart += 2;

		// Stash the overflow size
		size_t rest = (mb->wr_ptr() - pStart);

		// See if we have finished
		if (chunk_len == 0)
		{
			if (rest != 0)
			{
				HTTPRespondError("400 Bad Request");
				return;
			}

			// Adjust the current buffer
			mb->rd_ptr((char*)pEnd);
			mb->wr_ptr((char*)pEnd + chunk_len);
			break;
		}

		// Create a copy of the current block
		ACE_Message_Block* pNewBuffer = mb->duplicate();

		// Adjust the buffers
		mb->rd_ptr((char*)pEnd);
		mb->wr_ptr((char*)pEnd + chunk_len);
		pNewBuffer->rd_ptr((char*)pStart);

		// Update the buffers
		mb->cont(pNewBuffer);
		mb = pNewBuffer;
	}

	// If we get here, then we have all the message

	// Back to a ready state
	m_read_state = request_header;

	// Let the manager handle it
	m_pManager->handle_http_request(this,m_conn_id);

	if (m_mbRecv->cont())
	{
		m_mbRecv->cont()->release();
		m_mbRecv->cont(0);
	}
	m_mbRecv->reset();
}

void User::HttpConnection::Recv(const ACE_Message_Block* mb_in, ACE_CDR::ULong seq_no)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	if (guard.locked () == 0)
	{
		HTTPRespondError("500 Internal Server Error",ACE_OS::last_error());
		return;
	}

	if (seq_no != m_seq_no)
	{
		m_mapOutOfSequence.insert(std::map<ACE_CDR::ULong,ACE_Message_Block*>::value_type(seq_no,mb_in->duplicate()));
		return;
	}

	// Loop processing packets
	ACE_Message_Block* mb = mb_in->duplicate();

	for (;;)
	{
		// Now loop parsing
		for (bool bContinue = true;bContinue;)
		{
			bContinue = false;
			switch (m_read_state)
			{
			case request_header:
				bContinue = ReadRequestHeader(mb);
				mb->release();
				mb = 0;
				break;

			case request_data:
				ReadRequestData(mb);
				break;

			case request_chunked:
				ReadRequestChunked(mb);
				break;

			default:
				HTTPRespondError("500 Internal Server Error",EINVAL);
				break;
			}
		}

		if (mb)
			mb->release();

		++m_seq_no;

		std::map<ACE_CDR::ULong,ACE_Message_Block*>::iterator i=m_mapOutOfSequence.find(m_seq_no);
		if (i == m_mapOutOfSequence.end())
			break;

		mb = i->second;
		m_mapOutOfSequence.erase(i);
	}
}

void User::HttpConnection::HTTPRespondError(const char* pszMessage, int err, const char* pszHeaders)
{
	std::ostringstream osResponse;
	osResponse <<
		"<html>"
			"<head>"
				"<META content=NOINDEX name=ROBOTS>"
				"<title>The page cannot be displayed</title>"
			"</head>"
			"<body>";

	osResponse << pszMessage;
	if (err != 0)
		osResponse << "<br>" << ACE_OS::strerror(err);

	osResponse <<
			"</body>"
		"</html>";

	std::string strResponse = osResponse.str();

	std::ostringstream osHeader;
	osHeader << "HTTP/1.1 " << pszMessage << "\r\n";
	osHeader << "Server: OOServer/" OOCORE_VERSION "\r\n";
	osHeader << "Connection: close\r\n";
	osHeader << "Content-Type: text/html; charset=utf-8\r\n";
	osHeader << "Content-Length: " << (uint64_t)strResponse.length() << "\r\n";

	if (pszHeaders)
		osHeader << pszHeaders;

	osHeader << "\r\n";
	std::string strHeader = osHeader.str();

	ACE_Message_Block mb(strResponse.length() + strHeader.length());
	mb.copy(strHeader.c_str(),strHeader.length());
	mb.copy(strResponse.c_str(),strResponse.length());
	SendToRoot(&mb);

	// Close the connection
	Close();
}

void User::HttpConnection::HTTPRedirect(string_t strResource, bool bWithContent, const std::string& strHeaders)
{
	bool bClose = false;
	if (strResource[0] != L'/')
	{
		string_t strScheme, strHost, strPort, strUserName, strPassword, strResource2, strQuery;
		Net::Http::SplitURL(strResource,strScheme,strHost,strPort,strUserName,strPassword,strResource2,strQuery);

		bClose = (strHost.ToLower() != GetRequestHeader(L"Host").ToLower());
	}
	else
	{
		strResource = BaseURI() + strResource;
	}

	// Compose a JavaScript redirecting response
	std::ostringstream osResponse;
	if (bWithContent)
	{
		osResponse <<
			"<html>"
				"<head>"
					"<META content=NOINDEX name=ROBOTS>"
				"</head>"
				"<body>"
					"<script type=\"text/javascript\">"
					"<!-- "
					"window.location = \""
					<< strResource.ToUTF8() <<
					"\" "
					"//-->"
					"</script>"
				"</body>"
			"</html>";
	}
	std::string strResponse = osResponse.str();

	std::ostringstream osHeader;
	osHeader <<
		"HTTP/1.1 302 Found\r\n"
		"Server: OOServer/" OOCORE_VERSION "\r\n"
		"Location: " << strResource.ToUTF8() << "\r\n";

	if (bClose)
		osHeader << "Connection: close\r\n";

	osHeader << "Content-Length: " << (uint64_t)strResponse.length() << "\r\n";

	if (bWithContent)
		osHeader << "Content-Type: text/html; charset=utf-8\r\n";

	if (!strHeaders.empty())
		osHeader << strHeaders;

	osHeader << "\r\n";
	std::string strHeader = osHeader.str();

	ACE_Message_Block mb(strResponse.length() + strHeader.length());
	mb.copy(strHeader.c_str(),strHeader.length());
	mb.copy(strResponse.c_str(),strResponse.length());
	SendToRoot(&mb);

	if (bClose)
	{
		// Close the connection
		Close();
	}
}

void User::HttpConnection::Close()
{
	ACE_OutputCDR message;
	message << static_cast<Root::RootOpCode_t>(Root::HttpClose);
	message.write_ushort(m_conn_id);
	if (!message.good_bit())
		return;

	try
	{
		m_pManager->sendrecv_root(message,TypeInfo::Asynchronous);
	}
	catch (IException* pE)
	{
		pE->Release();
	}
}

bool User::HttpConnection::SendToRoot(const ACE_Message_Block* mb)
{
	ACE_OutputCDR message;
	message << static_cast<Root::RootOpCode_t>(Root::HttpSend);
	message.write_ushort(m_conn_id);
	message.write_octet_array_mb(mb);
	if (!message.good_bit())
		return false;

	try
	{
		m_pManager->sendrecv_root(message,TypeInfo::Asynchronous);
		return true;
	}
	catch (IException* pE)
	{
		pE->Release();
		return false;
	}
}

User::RequestStream::RequestStream() :
	m_mb(0)
{
}

User::RequestStream::~RequestStream()
{
	if (m_mb)
		m_mb->release();
}

void User::RequestStream::init(const ACE_Message_Block* mb)
{
	if (mb)
		m_mb = mb->duplicate();
}

void User::RequestStream::ReadBytes(uint64_t& cbBytes, byte_t* val)
{
	OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

	if (!m_mb)
	{
		cbBytes = 0;
		return;
	}

	if (!val)
	{
		cbBytes = (uint32_t)m_mb->total_length();
		return;
	}

	if (cbBytes > m_mb->total_length())
		cbBytes = (uint32_t)m_mb->total_length();

	for (uint64_t cb = cbBytes; m_mb != 0 && cb != 0;)
	{
		size_t cb2 = m_mb->length();
		if (cb2 > cb)
			cb2 = (size_t)cb;

		ACE_OS::memcpy(val,m_mb->rd_ptr(),cb2);
		val += cb2;
		cb -= (uint32_t)cb2;
		m_mb->rd_ptr(cb2);

		if (m_mb->length() == 0)
		{
			ACE_Message_Block* mbn = m_mb->cont();
			m_mb->cont(0);
			m_mb->release();
			m_mb = mbn;
		}
	}
}

void User::RequestStream::WriteBytes(const uint64_t&, const byte_t*)
{
	OMEGA_THROW(EACCES);
}

User::HttpRequest::HttpRequest() :
	m_mbRecv(0)
{
}

User::HttpRequest::~HttpRequest()
{
	if (m_mbRecv)
		m_mbRecv->release();
}

void User::HttpRequest::init(HttpConnection* pConn, const string_t& strResource)
{
	m_strMethod = pConn->m_strMethod;
	m_strResource = strResource;
	m_mapRequestHeaders = pConn->m_mapRequestHeaders;
	m_mbRecv = pConn->m_mbRecv->duplicate();
}

string_t User::HttpRequest::Method()
{
	return m_strMethod;
}

string_t User::HttpRequest::Resource()
{
	return m_strResource;
}

Omega::IEnumString* User::HttpRequest::GetAllRequestHeaders()
{
	ObjectPtr<ObjectImpl<EnumString> > ptrEnum = ObjectImpl<EnumString>::CreateInstancePtr();

	for (std::map<string_t,HttpConnection::Header>::const_iterator i = m_mapRequestHeaders.begin();i!=m_mapRequestHeaders.end();++i)
		ptrEnum->Append(i->second.strHeader);

	ptrEnum->Init();
	return ptrEnum.AddRef();
}

string_t User::HttpRequest::GetRequestHeader(const string_t& strHeader)
{
	std::map<string_t,HttpConnection::Header>::const_iterator i = m_mapRequestHeaders.find(strHeader.ToLower());
	if (i == m_mapRequestHeaders.end())
		return string_t();

	return i->second.strValue;
}

void User::HttpRequest::RequestBody(uint32_t& cbBytes, byte_t* pBody)
{
	if (!m_mbRecv)
	{
		cbBytes = 0;
		return;
	}

	if (!pBody)
	{
		cbBytes = (uint32_t)m_mbRecv->total_length();
		return;
	}

	if (cbBytes > m_mbRecv->total_length())
		cbBytes = (uint32_t)m_mbRecv->total_length();

	ACE_Message_Block* pCur = m_mbRecv;
	for (uint32_t cb = cbBytes; cb && pCur;)
	{
		size_t cb2 = pCur->length();
		if (cb2 > cb)
			cb2 = cb;

		ACE_OS::memcpy(pBody,pCur->rd_ptr(),cb2);
		pBody += cb2;
		cb -= (uint32_t)cb2;

		pCur = pCur->cont();
	}
}

IO::IStream* User::HttpRequest::RequestStream()
{
	if (!m_mbRecv)
		return 0;

	ObjectPtr<ObjectImpl<User::RequestStream> > ptrStream = ObjectImpl<User::RequestStream>::CreateInstancePtr();
	ptrStream->init(m_mbRecv);

	return ptrStream.AddRef();
}

User::ResponseStream::ResponseStream() :
	m_pManager(0), m_conn_id(0), m_bClose(false), m_bHasLength(false), m_content_length(0)
{
}

User::ResponseStream::~ResponseStream()
{
	if (m_bClose)
	{
		// Send the close
		ACE_OutputCDR message;
		message << static_cast<Root::RootOpCode_t>(Root::HttpClose);
		message.write_ushort(m_conn_id);
		if (message.good_bit())
			m_pManager->sendrecv_root(message,TypeInfo::Asynchronous);
	}
}

void User::ResponseStream::init(Manager* pManager, uint16_t conn_id, bool bClose, unsigned long length)
{
	m_pManager = pManager;
	m_conn_id = conn_id;
	m_bClose = bClose;
	m_content_length = length;
	m_bHasLength = (length != 0);
}

void User::ResponseStream::ReadBytes(uint64_t&, byte_t*)
{
	OMEGA_THROW(EACCES);
}

void User::ResponseStream::WriteBytes(const uint64_t& cbBytes, const byte_t* val)
{
	ACE_CDR::ULong cb = static_cast<ACE_CDR::ULong>(cbBytes);

	if (m_bHasLength)
	{
		if (cb > m_content_length.value())
			OMEGA_THROW(L"Attempting to write more than Content-Length");

		if (cb == 0)
			OMEGA_THROW(E2BIG);

		m_content_length -= cb;
	}

	ACE_OutputCDR message;
	message << static_cast<Root::RootOpCode_t>(Root::HttpSend);
	message.write_ushort(m_conn_id);
	message.write_octet_array(val,cb);
	if (!message.good_bit())
		OMEGA_THROW(ACE_OS::last_error());

	m_pManager->sendrecv_root(message,TypeInfo::Asynchronous);
}

User::HttpResponse::HttpResponse() :
	m_pManager(0),
	m_conn_id(0)
{
}

void User::HttpResponse::init(Manager* pManager, uint16_t conn_id)
{
	m_pManager = pManager;
	m_conn_id = conn_id;

	SetResponseHeader(L"Server",L"OOServer/" + string_t(OOCORE_VERSION,false));
}

void User::HttpResponse::SetResponseHeader(const string_t& strHeader, const string_t& strValue)
{
	OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

	HttpConnection::Header h;
	h.strHeader = strHeader.TrimLeft(L' ').TrimRight(L" :");
	h.strValue = strValue.TrimLeft(L' ').TrimRight(L" \r\n");

	// If empty string, remove, else update
	if (h.strValue.IsEmpty())
		m_mapResponseHeaders.erase(h.strHeader.ToLower());
	else
		m_mapResponseHeaders[h.strHeader.ToLower()] = h;
}

IO::IStream* User::HttpResponse::Send(uint16_t uStatus, const string_t& strStatusText)
{
	OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

	// Compose the message
	std::ostringstream osHeader;
	osHeader << "HTTP/1.1 " << uStatus << " " << strStatusText.ToUTF8() << "\r\n";

	bool bClose = false;
	bool bNoResponse = true;
	unsigned long content_length = 0;
	for (std::map<string_t,HttpConnection::Header>::const_iterator i=m_mapResponseHeaders.begin();i!=m_mapResponseHeaders.end();++i)
	{
		// Check for content
		if (i->first == L"content-length")
		{
			content_length = ACE_OS::strtoul(i->second.strValue.c_str(),NULL,10);
			if (content_length != 0)
				bNoResponse = false;
		}
		else if (i->first == L"transfer-encoding" && i->second.strValue == L"chunked")
		{
			bNoResponse = false;
		}

		// Check for close
		if (i->first == L"connection" && i->second.strValue == L"close")
			bClose = true;

		osHeader <<  i->second.strHeader.ToUTF8() << ": " << i->second.strValue.ToUTF8() << "\r\n";
	}
	osHeader << "\r\n";
	std::string strHeader = osHeader.str();

	// Add the header length, because we subtract as we go...
	if (content_length)
		content_length += static_cast<unsigned long>(strHeader.length());

	// Create a response stream...
	ObjectPtr<ObjectImpl<User::ResponseStream> > ptrStream = ObjectImpl<User::ResponseStream>::CreateInstancePtr();
	ptrStream->init(m_pManager,m_conn_id,bClose,content_length);

	// Send the message
	ptrStream->WriteBytes(strHeader.length(),(const byte_t*)strHeader.c_str());

	// If we have no length or chunked, return 0
	if (bNoResponse)
		return 0;

	// Return a stream to send extra data
	return ptrStream.AddRef();
}

void User::Manager::open_http(ACE_InputCDR& request, ACE_OutputCDR& response)
{
	ACE_CDR::UShort conn_id = 0;
	request.read_ushort(conn_id);

	ACE_CString strRemoteAddr, strScheme;
	request.read_string(strRemoteAddr);
	request.read_string(strScheme);

	int err = 0;
	if (!request.good_bit())
		err = ACE_OS::last_error();
	else
	{
		// Create a new connection
		HttpConnection* pConn = 0;
		ACE_NEW_NORETURN(pConn,HttpConnection(this,conn_id,strRemoteAddr,strScheme));
		if (!pConn)
			err = ENOMEM;
		else
		{
			ACE_Refcounted_Auto_Ptr<HttpConnection,ACE_Thread_Mutex> new_conn(pConn);

			// Insert into map
			ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_http_lock);
			if (guard.locked() == 0)
				err = ACE_OS::last_error();
			else
			{
				try
				{
					// Okay, got a new HTTP connection
					m_mapHttpConnections.insert(std::map<uint16_t,ACE_Refcounted_Auto_Ptr<HttpConnection,ACE_Thread_Mutex> >::value_type(conn_id,new_conn));
				}
				catch (std::exception&)
				{
					err = EINVAL;
				}
			}
		}
	}

	response << err;
}

void User::Manager::recv_http(ACE_InputCDR& request)
{
	ACE_CDR::UShort conn_id = 0;
	request.read_ushort(conn_id);

	int err = 0;
	ACE_CDR::ULong seq_no = 0;

	request >> err;
	if (err == 0)
		request >> seq_no;

	if (!request.good_bit() && err == 0)
		err = -1;

	if (err == 0)
	{
		// Okay, got a HTTP message, who is it from?
		ACE_Refcounted_Auto_Ptr<HttpConnection,ACE_Thread_Mutex> conn;

		// Lookup connection
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_http_lock);
		if (guard.locked() != 0)
		{
			try
			{
				// Okay, got a new HTTP connection
				std::map<uint16_t,ACE_Refcounted_Auto_Ptr<HttpConnection,ACE_Thread_Mutex> >::iterator i=m_mapHttpConnections.find(conn_id);
				if (i != m_mapHttpConnections.end())
					conn = i->second;
			}
			catch (std::exception&)
			{
			}
		}
		guard.release();

		if (!conn.null())
		{
			// We have a valid connection
			conn->Recv(request.start(),seq_no);
		}
	}
	else
	{
		try
		{
			ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_http_lock);

			m_mapHttpConnections.erase(conn_id);
		}
		catch (std::exception&)
		{
		}
	}
}

void User::Manager::handle_http_request(HttpConnection* pConn, uint16_t conn_id)
{
	// Check we have something at least
	if (pConn->m_strResource[0] != L'/')
	{
		pConn->HTTPRespondError("400 Bad Request");
	}
	else if (pConn->m_strMethod == L"GET" || pConn->m_strMethod == L"HEAD" || pConn->m_strMethod == L"POST")
	{
		if (pConn->m_strResource == L"*")
			pConn->HTTPRespondError("400 Bad Request");
		else
		{
			try
			{
				handle_http_request_i(pConn,conn_id);
			}
			catch (IException* pE)
			{
				pE->Release();
				pConn->HTTPRespondError("500 Internal Server Error");
			}
			catch (...)
			{
				pConn->HTTPRespondError("500 Internal Server Error");
			}
		}
	}
	else if (pConn->m_strMethod == L"OPTIONS")
	{
		void* TODO;
	}
	else if (pConn->m_strMethod == L"PUT" ||
		pConn->m_strMethod == L"DELETE" ||
		pConn->m_strMethod == L"CONNECT" ||
		pConn->m_strMethod == L"TRACE")
	{
		pConn->HTTPRespondError("405 Method Not Allowed",0,"Allow: GET, HEAD, POST, OPTIONS");
	}
	else
	{
		pConn->HTTPRespondError("501 Not Implemented");
	}
}

void User::Manager::handle_http_request_i(HttpConnection* pConn, uint16_t conn_id)
{
	// Select a default oid
	string_t strResource = pConn->m_strResource;
	if (strResource == L"/")
	{
		// Redirect to the default... (the one in OORpc)
		strResource = L"/aea785bc-47b2-451b-acff-61c1dee1ad25/";

		// Get the scheme, and lookup in the registry
		string_t strScheme(pConn->m_strScheme.c_str(),false);
		size_t pos = strScheme.Find(L"://");
		if (pos != string_t::npos)
			strScheme = strScheme.Left(pos);

		ObjectPtr<Registry::IKey> ptrKey(L"\\Server");
		if (ptrKey->IsSubKey(strScheme))
		{
			ptrKey = ptrKey.OpenSubKey(strScheme);
			if (ptrKey->IsValue(L"DefaultURI"))
			{
				strResource = ptrKey->GetStringValue(L"DefaultURI");
				if (strResource.Left(1) != L"/")
					strResource = L"/" + strResource;

				if (strResource.Right(1) != L"/")
					strResource += L"/";
			}
		}

		// Redirect to the default URI
		return pConn->HTTPRedirect(strResource,pConn->m_strMethod != L"HEAD");
	}

	// Find the OID
	string_t strOID;
	size_t pos = strResource.Find(L'/',1);
	if (pos != string_t::npos)
	{
		strOID = strResource.Mid(1,pos-1);
		strResource = strResource.Mid(pos);
	}
	else
	{
		strOID = strResource.Mid(1);
		strResource = L"/";
	}

	// Create a fully qualified URI
	string_t strURI = pConn->BaseURI() + L"/" + strOID;

	bool bNew = false;
	ObjectPtr<Net::Http::Server::IRequestHandler> ptrHandler;

	// Try to resolve the strOID to an actual oid
	try
	{
		guid_t oid = guid_t::FromString(L"{" + strOID + L"}");
		if (oid == guid_t::Null())
			oid = Activation::NameToOid(strOID);
		else
		{
			// Force all oid strings to lower
			strURI = pConn->BaseURI() + L"/" + strOID.ToLower();
		}

		// See if we have one already...
		{
			OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_http_lock);

			std::map<string_t,ObjectPtr<Net::Http::Server::IRequestHandler> >::iterator i=m_mapHttpHandlers.find(strURI);
			if (i != m_mapHttpHandlers.end())
				ptrHandler = i->second;
		}

		if (!ptrHandler)
		{
			// Create a new one and insert
			ptrHandler = ObjectPtr<Net::Http::Server::IRequestHandler>(oid);

			OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_http_lock);

			std::pair<std::map<string_t,ObjectPtr<Net::Http::Server::IRequestHandler> >::iterator,bool> p=m_mapHttpHandlers.insert(std::map<string_t,ObjectPtr<Net::Http::Server::IRequestHandler> >::value_type(strURI,ptrHandler));
			bNew = p.second;
			if (!bNew)
				ptrHandler = p.first->second;
		}
	}
	catch (IException* pE)
	{
		pE->Release();
		return pConn->HTTPRespondError("404 Not Found");
	}

	// Init the handler if its new...
	if (bNew)
		ptrHandler->Open(strURI);

	// Create a request object
	ObjectPtr<ObjectImpl<HttpRequest> > ptrRequest = ObjectImpl<HttpRequest>::CreateInstancePtr();
	ptrRequest->init(pConn,strResource);

	// Create a response object
	ObjectPtr<ObjectImpl<HttpResponse> > ptrResponse = ObjectImpl<HttpResponse>::CreateInstancePtr();
	ptrResponse->init(this,conn_id);

	// Call the handler
	ptrHandler->ProcessRequest(ptrRequest,ptrResponse);
}

void User::Manager::close_all_http()
{
	std::map<uint16_t,ACE_Refcounted_Auto_Ptr<HttpConnection,ACE_Thread_Mutex> > conns;
	std::map<string_t,ObjectPtr<Net::Http::Server::IRequestHandler> > handlers;

	// Make a locked copy of the maps and clear them
	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_http_lock);

		conns = m_mapHttpConnections;
		m_mapHttpConnections.clear();

		handlers = m_mapHttpHandlers;
		m_mapHttpHandlers.clear();
	}

	conns.clear();
	handlers.clear();
}
