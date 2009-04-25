///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OONet_precomp.h"

#include "HttpImpl.h"

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	class HttpStream :
		public ObjectBase,
		public IO::IStream
	{
	public:
		HttpStream();
		virtual ~HttpStream();

		void init(ACE_Message_Block** mb, OOBase::Mutex* lock);

		BEGIN_INTERFACE_MAP(HttpStream)
			INTERFACE_ENTRY(IO::IStream)
		END_INTERFACE_MAP()

	private:
		HttpStream(const HttpStream&) : m_lock(0), m_mb(0) {}
		HttpStream& operator =(const HttpStream&) { return *this; }

		OOBase::Mutex*      m_lock;
		ACE_Message_Block** m_mb;

	public:
		void ReadBytes(uint64_t& cbBytes, byte_t* val);
		void WriteBytes(const uint64_t& cbBytes, const byte_t* val);
	};

	class HttpBase
	{
	public:
		HttpBase();

		bool open(const string_t& strMethod, const string_t& strURL);

	protected:
		void SetRequestHeader(const string_t& strHeader, const string_t& strValue);
		string_t GetResponseHeader(const string_t& strHeader);
		string_t ComposeRequest(uint32_t cbBytes);
		uint64_t CheckHTTPHeader(ACE_Message_Block* pBuffer);
		void SplitHTTPHeader(ACE_Message_Block* pBuffer);
		void ParseResponseStatus(ACE_Message_Block* pBuffer);
		uint64_t ParseChunks(ACE_Message_Block*& pBuffer);
		void grow_mb(ACE_Message_Block* pBuffer, size_t cbBytes);
		Omega::IEnumString* GetAllResponseHeaders();

		string_t          m_strAddr;
		string_t          m_strMethod;
		string_t          m_strResource;
		uint16_t          m_uStatus;
		string_t          m_strStatusText;

	private:
		struct Header
		{
			string_t strHeader;
			string_t strValue;
		};
		std::map<string_t,Header> m_mapRequestHeaders;
		std::map<string_t,Header> m_mapResponseHeaders;

		std::string Trim(const std::string& str);
		inline char* QuickFind(ACE_Message_Block* pBuffer, char* pStart, char c);
		std::string NextLine(ACE_Message_Block* pBuffer);
	};

	class HttpRequestSync :
		public ObjectBase,
		public HttpBase,
		public Net::Http::IRequest
	{
	public:
		HttpRequestSync();
		virtual ~HttpRequestSync();

		BEGIN_INTERFACE_MAP(HttpRequestSync)
			INTERFACE_ENTRY(Net::Http::IRequest)
		END_INTERFACE_MAP()

	private:
		OOBase::Mutex          m_lock;
		ObjectPtr<IO::IStream> m_ptrStream;
		ACE_Message_Block*     m_mb;

		void ReadResponse();
		void Send_i(uint32_t cbBytes, const byte_t* pData, std::set<Omega::string_t>& setRedirects);

	// IRequest members
	public:
		void Open(const string_t& strMethod, const string_t& strURL, Net::Http::IRequestNotify* pAsyncNotify);
		void SetRequestHeader(const string_t& strHeader, const string_t& strValue);
		void Send(uint32_t cbBytes, const byte_t* pData);
		uint16_t Status();
		string_t StatusText();
		Omega::IEnumString* GetAllResponseHeaders();
		string_t GetResponseHeader(const string_t& strHeader);
		void ResponseBody(uint32_t& cbBytes, byte_t* pBody);
		IO::IStream* ResponseStream();
		void Abort();
		bool_t WaitForResponse(uint32_t timeout);
	};

	class HttpRequestAsync :
		public ObjectBase,
		public HttpBase,
		public Net::Http::IRequest,
		public IO::IAsyncStreamNotify
	{
	public:
		HttpRequestAsync();
		virtual ~HttpRequestAsync();

		BEGIN_INTERFACE_MAP(HttpRequestAsync)
			INTERFACE_ENTRY(Net::Http::IRequest)
			INTERFACE_ENTRY(IO::IAsyncStreamNotify)
		END_INTERFACE_MAP()

	private:
		OOBase::Mutex                        m_lock;
		ObjectPtr<IO::IStream>               m_ptrStream;
		ObjectPtr<Net::Http::IRequestNotify> m_ptrNotify;
		ACE_Message_Block*                   m_mbRequest;
		ACE_Message_Block*                   m_mbBuffer;
		ACE_Message_Block*                   m_mbResponse;
		uint32_t                             m_content_length;
		ACE_Event                            m_completion;
		std::set<Omega::string_t>            m_setRedirects;

		enum ReadState
		{
			closed,
			ready_to_send,
			sending_request,
			response_header,
			response_data,
			response_chunked
		} m_read_state;

		void Send_i();
		void SendRequest();
		void ResetState();

		bool ReadResponseHeader(unsigned int& notify_mask);
		void ReadResponseData(unsigned int& notify_mask);
		void ReadResponseChunked(unsigned int& notify_mask);

	// IAsyncNotify members
	public:
		void OnOpened();
		void OnRead(const uint64_t& cbBytes, const byte_t* pData);
		void OnWritten(const uint64_t& cbBytes);
		void OnError(IException* pE);

	// IRequest members
	public:
		void Open(const string_t& strMethod, const string_t& strURL, Net::Http::IRequestNotify* pAsyncNotify);
		void SetRequestHeader(const string_t& strHeader, const string_t& strValue);
		void Send(uint32_t cbBytes, const byte_t* pData);
		uint16_t Status();
		string_t StatusText();
		Omega::IEnumString* GetAllResponseHeaders();
		string_t GetResponseHeader(const string_t& strHeader);
		void ResponseBody(uint32_t& cbBytes, byte_t* pBody);
		IO::IStream* ResponseStream();
		void Abort();
		bool_t WaitForResponse(uint32_t timeout);
	};
}

OOCore::HttpStream::HttpStream() :
	m_lock(0),
	m_mb(0)
{
}

OOCore::HttpStream::~HttpStream()
{
}

void OOCore::HttpStream::init(ACE_Message_Block** mb, OOBase::Mutex* lock)
{
	m_lock = lock;
	m_mb = mb;
}

void OOCore::HttpStream::ReadBytes(uint64_t& cbBytes, byte_t* val)
{
	OOBase::Guard<OOBase::Mutex> guard(*m_lock);
	
	if (!m_mb)
	{
		cbBytes = 0;
		return;
	}

	if (!val)
	{
		cbBytes = (*m_mb)->total_length();
		return;
	}

	if (cbBytes > (*m_mb)->total_length())
		cbBytes = (*m_mb)->total_length();

	for (uint64_t cb = cbBytes; m_mb != 0 && cb != 0;)
	{
		size_t cb2 = (*m_mb)->length();
		if (cb2 > cb)
			cb2 = (size_t)cb;

		ACE_OS::memcpy(val,(*m_mb)->rd_ptr(),cb2);
		val += cb2;
		cb -= cb2;
		(*m_mb)->rd_ptr(cb2);

		if ((*m_mb)->length() == 0)
		{
			ACE_Message_Block* mbn = (*m_mb)->cont();
			(*m_mb)->cont(0);
			(*m_mb)->release();
			*m_mb = mbn;
		}
	}
}

void OOCore::HttpStream::WriteBytes(const uint64_t&, const byte_t*)
{
	OMEGA_THROW(EACCES);
}

OOCore::HttpBase::HttpBase()
{
	SetRequestHeader(L"User-Agent",L"OOCore " + string_t(OOCORE_VERSION,false));
}

void OOCore::HttpBase::grow_mb(ACE_Message_Block* pBuffer, size_t cbBytes)
{
	if (pBuffer->space() >= cbBytes)
		return;

	if (pBuffer->size(pBuffer->size() + cbBytes) == -1)
		OMEGA_THROW(ACE_OS::last_error());
}

bool OOCore::HttpBase::open(const string_t& strMethod, const string_t& strURL)
{
	string_t strOldAddr = m_strAddr;

	m_strMethod = strMethod;

	// Parse the request out to its bits...
	string_t strScheme, strHost, strPort, strUserName, strPassword, strResource, strQuery;
	Net::Http::SplitURL(strURL,strScheme,strHost,strPort,strUserName,strPassword,strResource,strQuery);

	if (strScheme.IsEmpty())
		strScheme = L"http";
	else if (strScheme!=L"http" && strScheme!=L"https")
		OMEGA_THROW(L"Invalid protocol!");

	m_strAddr = strScheme + L"://";

	/*if (!strUserName.IsEmpty())
	{
		m_strAddr += strUserName;
		if (!strPassword.IsEmpty())
			m_strAddr += L":" + strPassword;

		m_strAddr += L"@";
	}*/

	if (!strPort.IsEmpty())
		strHost += L":" + strPort;

	m_strAddr += strHost;
	m_strResource = strResource;
	if (!strQuery.IsEmpty())
		m_strResource += L"?" + strQuery;

	SetRequestHeader(L"Host",strHost);

	return (m_strAddr != strOldAddr);
}

void OOCore::HttpBase::SetRequestHeader(const string_t& strHeader, const string_t& strValue)
{
	Header h;
	h.strHeader = strHeader;
	h.strValue = strValue;

	// Remove : from the header
	size_t pos = h.strHeader.Find(L':');
	if (pos != string_t::npos)
		h.strHeader = h.strHeader.Left(pos);

	// Remove any line feeds from the value
	pos = h.strValue.Find(L"\r\n");
	if (pos != string_t::npos)
		h.strValue = h.strValue.Left(pos);

	if (h.strValue.IsEmpty())
		m_mapRequestHeaders.erase(h.strHeader.ToLower());
	else
		m_mapRequestHeaders[h.strHeader.ToLower()] = h;
}

string_t OOCore::HttpBase::GetResponseHeader(const string_t& strHeader)
{
	std::map<string_t,Header>::iterator i = m_mapResponseHeaders.find(strHeader.ToLower());
	if (i == m_mapResponseHeaders.end())
		return string_t();

	return i->second.strValue;
}

Omega::IEnumString* OOCore::HttpBase::GetAllResponseHeaders()
{
	ObjectPtr<ObjectImpl<EnumString> > ptrEnum = ObjectImpl<EnumString>::CreateInstancePtr();

	for (std::map<string_t,Header>::iterator i = m_mapResponseHeaders.begin();i!=m_mapResponseHeaders.end();++i)
		ptrEnum->Append(i->second.strHeader);

	ptrEnum->Init();
	return ptrEnum.AddRef();
}

string_t OOCore::HttpBase::ComposeRequest(uint32_t cbBytes)
{
	string_t strRequest = m_strMethod + L" " + m_strResource + L" HTTP/1.1\r\n";

	// Set the date
	time_t now_t = ACE_OS::gettimeofday().sec();
	tm now;
	ACE_OS::gmtime_r(&now_t,&now);
	char szBuf[160];
	ACE_OS::strftime(szBuf,160,"%a, %d %b %Y %H:%M:%S GMT\r\n",&now);
	SetRequestHeader(L"Date",string_t(szBuf,false));

	// Check content length
	if (cbBytes)
	{
		std::map<string_t,Header>::iterator i = m_mapRequestHeaders.find(L"transfer-encoding");
		if (i == m_mapRequestHeaders.end())
		{
			SetRequestHeader(L"Content-Length",string_t::Format(L"%lu",cbBytes));
		}
	}

	for (std::map<string_t,Header>::iterator i=m_mapRequestHeaders.begin();i!=m_mapRequestHeaders.end();++i)
	{
		strRequest += i->second.strHeader + L": " + i->second.strValue + L"\r\n";
	}

	strRequest += L"\r\n";
	return strRequest;
}

std::string OOCore::HttpBase::Trim(const std::string& str)
{
	size_t start = str.find_first_not_of(' ');
	if (start == std::string::npos)
		start = 0;

	size_t end = str.find_last_not_of(' ');
	return str.substr(start,end+1);
}

char* OOCore::HttpBase::QuickFind(ACE_Message_Block* pBuffer, char* pStart, char c)
{
	char* pEnd = pStart;
	while (*pEnd != c && pEnd < pBuffer->wr_ptr())
		++pEnd;

	return (*pEnd == c ? pEnd : 0);
}

uint64_t OOCore::HttpBase::CheckHTTPHeader(ACE_Message_Block* pBuffer)
{
	if (pBuffer->length() == 0)
	{
		// We definitely need more
		return 256;
	}

	char* pStart = pBuffer->rd_ptr();
	
	// Check we have HTTP first
	if (pBuffer->length() >= 9)
	{
		if (ACE_OS::strncmp(pStart,"HTTP/1.1 ",9) != 0)
			return (uint64_t)-1;
	}

	for (;;)
	{
		char* pEnd = QuickFind(pBuffer,pStart,'\r');
		if (!pEnd)
		{
			// We definitely need more
			return 256;
		}

		if (pEnd - pStart == 0)
		{
			// We have got everything we need
			break;
		}

		// Start at next line...
		pStart = pEnd + 2;
	}

	return 0;
}

std::string OOCore::HttpBase::NextLine(ACE_Message_Block* pBuffer)
{
	const char* pStart = pBuffer->rd_ptr();
	const char* pEnd = QuickFind(pBuffer,pBuffer->rd_ptr(),'\r');
	if (pEnd)
		pBuffer->rd_ptr(pEnd - pStart + 2);
	else
	{
		pEnd = pBuffer->wr_ptr();
		pBuffer->rd_ptr(pEnd - pStart);
	}

	return std::string(pStart,pEnd - pStart);
}

void OOCore::HttpBase::ParseResponseStatus(ACE_Message_Block* pBuffer)
{
	std::string strHeader = NextLine(pBuffer);
	size_t pos = strHeader.find(' ');
	if (pos != std::string::npos)
		pos = strHeader.find_first_not_of(' ',pos+1);

	if (pos == std::string::npos)
		OMEGA_THROW(L"Invalid HTTP header");

	strHeader = strHeader.substr(pos);
	pos = strHeader.find(' ');
	if (pos == std::string::npos)
		m_uStatus = (uint16_t)ACE_OS::strtoul(strHeader.c_str(),0,10);
	else
	{
		m_uStatus = (uint16_t)ACE_OS::strtoul(strHeader.substr(0,pos).c_str(),0,10);
		m_strStatusText = string_t(Trim(strHeader.substr(pos+1)).c_str(),false);
	}
}

void OOCore::HttpBase::SplitHTTPHeader(ACE_Message_Block* pBuffer)
{
	m_mapResponseHeaders.clear();

	for (;;)
	{
		std::string str = NextLine(pBuffer);
		if (str.empty())
			break;

		Header h;
		size_t pos = str.find(':');
		h.strHeader = string_t(Trim(str.substr(0,pos)).c_str(),false);

		if (pos < str.size()-1)
			h.strValue = string_t(Trim(str.substr(pos+1)).c_str(),false);

		m_mapResponseHeaders[h.strHeader.ToLower()] = h;
	}
}

uint64_t OOCore::HttpBase::ParseChunks(ACE_Message_Block*& pBuffer)
{
	for (;;)
	{
		// Find the first \r\n
		char* pStart = pBuffer->rd_ptr();
		char* pEnd = QuickFind(pBuffer,pStart,'\r');
		if (!pEnd)
		{
			if (pBuffer->space() < 512)
			{
				// Create a new buffer
				ACE_Message_Block* pNewBuffer = 0;
				OMEGA_NEW(pNewBuffer,ACE_Message_Block(ACE_OS::getpagesize()));
				pBuffer->cont(pNewBuffer);
				pBuffer = pNewBuffer;
			}
			return pBuffer->space();
		}

		// Find the chunk size bytes
		std::string strLen;
		const char* pExt = QuickFind(pBuffer,pStart,';');
		if (pExt)
			strLen = std::string(pStart,pExt-pStart);
		else
			strLen = std::string(pStart,pEnd-pStart);
		pEnd += 2;

		// Parse the chunk size
		uint32_t chunk_len = ACE_OS::strtoul(strLen.c_str(),NULL,16);

		// Check we have a whole chunk
		size_t len = pBuffer->length();
		size_t curr_len = (pEnd - pStart);
		if (len - curr_len < chunk_len + 2)
		{
			// No, we don't...
			return chunk_len + 2 - (len - curr_len);
		}

		// Move pStart to the next chunk start
		pStart = pEnd + chunk_len;
		if (pStart[0] != '\r' || pStart[1] != '\n')
			return (uint64_t)-1;
		pStart += 2;

		// Stash the overflow size
		size_t rest = (pBuffer->wr_ptr() - pStart);

		// See if we have finished
		if (chunk_len == 0)
		{
			if (rest != 0)
				return (uint64_t)-1;

			// Adjust the current buffer
			pBuffer->rd_ptr(pEnd);
			pBuffer->wr_ptr(pEnd + chunk_len);
			break;
		}

		// Create a copy of the current block
		ACE_Message_Block* pNewBuffer = pBuffer->duplicate();

		// Adjust the buffers
		pBuffer->rd_ptr(pEnd);
		pBuffer->wr_ptr(pEnd + chunk_len);
		pNewBuffer->rd_ptr(pStart);

		// Update the buffers
		pBuffer->cont(pNewBuffer);
		pBuffer = pNewBuffer;
	}

	return 0;
}

OOCore::HttpRequestSync::HttpRequestSync() :
	m_mb(0)
{
}

OOCore::HttpRequestSync::~HttpRequestSync()
{
	if (m_mb)
		m_mb->release();
}

void OOCore::HttpRequestSync::Send(uint32_t cbBytes, const byte_t* pData)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	std::set<Omega::string_t> setRedirects;

	Send_i(cbBytes,pData,setRedirects);

	// Read the response
	ReadResponse();
}

uint16_t OOCore::HttpRequestSync::Status()
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);
	
	return m_uStatus;
}

string_t OOCore::HttpRequestSync::StatusText()
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	return m_strStatusText;
}

Omega::IEnumString* OOCore::HttpRequestSync::GetAllResponseHeaders()
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	return HttpBase::GetAllResponseHeaders();
}

string_t OOCore::HttpRequestSync::GetResponseHeader(const string_t& strHeader)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	return HttpBase::GetResponseHeader(strHeader);
}

void OOCore::HttpRequestSync::ResponseBody(uint32_t& cbBytes, byte_t* pBody)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	if (!pBody)
	{
		cbBytes = (uint32_t)m_mb->total_length();
		return;
	}

	if (cbBytes > m_mb->total_length())
		cbBytes = (uint32_t)m_mb->total_length();

	ACE_Message_Block* pCur = m_mb;
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

IO::IStream* OOCore::HttpRequestSync::ResponseStream()
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	ObjectPtr<ObjectImpl<HttpStream> > ptrStream = ObjectImpl<HttpStream>::CreateInstancePtr();
	ptrStream->init(&m_mb,&m_lock);

	return ptrStream.AddRef();
}

void OOCore::HttpRequestSync::Abort()
{
	// Do nothing
}

bool_t OOCore::HttpRequestSync::WaitForResponse(uint32_t /*timeout*/)
{
	// We always succeeded, we are sync
	return true;
}

void OOCore::HttpRequestSync::Send_i(uint32_t cbBytes, const byte_t* pData, std::set<Omega::string_t>& setRedirects)
{
	// Compose
	std::string strRequest = ComposeRequest(cbBytes).ToUTF8();

	// Try to connect...
	if (!m_ptrStream)
		m_ptrStream.Attach(IO::OpenStream(m_strAddr));

	// Reset the buffer
	if (!m_mb)
	{
		OMEGA_NEW(m_mb,ACE_Message_Block(ACE_OS::getpagesize()));
	}
	else if (m_mb->cont())
	{
		m_mb->cont()->release();
		m_mb->cont(0);
	}
	m_mb->reset();

	// Make sure we have room
	grow_mb(m_mb,strRequest.length() + cbBytes);

	m_mb->copy(strRequest.c_str(),strRequest.length());
	if (cbBytes && pData)
		m_mb->copy((const char*)pData,cbBytes);

	// Send the data
	m_ptrStream->WriteBytes(m_mb->length(),(const byte_t*)m_mb->rd_ptr());

	// Reset the buffer before reading
	m_mb->reset();

	// Now loop, reading
	uint64_t cbRead = ACE_OS::getpagesize();
	while (cbRead)
	{
		// Make sure we have room
		grow_mb(m_mb,(size_t)cbRead);

		// Read some bytes
		m_ptrStream->ReadBytes(cbRead,(byte_t*)m_mb->wr_ptr());
		m_mb->wr_ptr((size_t)cbRead);

		// Try to parse the header...
		cbRead = CheckHTTPHeader(m_mb);
		if (cbRead == (uint64_t)-1)
			OMEGA_THROW(L"Failed to parse HTTP header");
	}

	// mb now contains a full response
	ParseResponseStatus(m_mb);
	SplitHTTPHeader(m_mb);

	// Check the status for redirects
	if (m_uStatus == 302 || m_uStatus == 307)
	{
		if (m_strMethod == L"GET" || m_strMethod == L"HEAD")
		{
			// Auto redirect
			string_t strLoc = HttpBase::GetResponseHeader(L"Location");

			// Open the new location
			bool bClose = open(m_strMethod,strLoc);
			if (bClose)
			{
				// Close the stream
				m_ptrStream.Release();
			}

			if (setRedirects.find(strLoc) != setRedirects.end())
				OMEGA_THROW(L"Redirection loop detected!");

			setRedirects.insert(strLoc);

			// Recurse into ourselves
			Send_i(cbBytes,pData,setRedirects);

			if (bClose)
				return;
		}
	}

	// Crunch up the buffer
	m_mb->crunch();
}

void OOCore::HttpRequestSync::ReadResponse()
{
	uint32_t content_length = ACE_OS::getpagesize();
	bool bChunked = false;

	string_t strC = HttpBase::GetResponseHeader(L"Content-Length");
	if (!strC.IsEmpty())
	{
		content_length = ACE_OS::strtoul(strC.c_str(),0,10);
		content_length -= (uint32_t)m_mb->length();
	}
	else
	{
		strC = HttpBase::GetResponseHeader(L"Transfer-Encoding");
		if (!strC.IsEmpty() && strC == L"chunked")
			bChunked = true;
		else
			content_length = 0;
	}

	ACE_Message_Block* pCurrMb = m_mb;

	// Now loop, reading
	uint64_t cbRead = content_length;
	while (cbRead)
	{
		// Make sure we have room
		grow_mb(pCurrMb,(size_t)cbRead);

		// Read some bytes
		m_ptrStream->ReadBytes(cbRead,(byte_t*)pCurrMb->wr_ptr());
		pCurrMb->wr_ptr((size_t)cbRead);

		if (!bChunked)
			break;

		// Try to parse the chunking...
		cbRead = ParseChunks(pCurrMb);
		if (cbRead == (uint32_t)-1)
			OMEGA_THROW(L"Failed to parse chunked encoding");
	}
}

void OOCore::HttpRequestSync::Open(const string_t& strMethod, const string_t& strURL, Net::Http::IRequestNotify*)
{
	open(strMethod,strURL);
}

void OOCore::HttpRequestSync::SetRequestHeader(const string_t& strHeader, const string_t& strValue)
{
	HttpBase::SetRequestHeader(strHeader,strValue);
}

OOCore::HttpRequestAsync::HttpRequestAsync() :
	m_mbRequest(0),
	m_mbBuffer(0),
	m_mbResponse(0),
	m_read_state(closed)
{
}

OOCore::HttpRequestAsync::~HttpRequestAsync()
{
	if (m_mbRequest)
		m_mbRequest->release();

	if (m_mbBuffer)
		m_mbBuffer->release();

	if (m_mbResponse)
		m_mbResponse->release();
}

void OOCore::HttpRequestAsync::Open(const string_t& strMethod, const string_t& strURL, Net::Http::IRequestNotify* pNotify)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Check whether we are ready to open
	if (m_read_state != closed && m_read_state != ready_to_send)
		OMEGA_THROW(EBUSY);

	m_ptrNotify = pNotify;

	if (open(strMethod,strURL))
	{
		// Close the stream
		m_ptrStream.Release();
	}

	// Now ready to send
	m_read_state = ready_to_send;
}

void OOCore::HttpRequestAsync::Send(uint32_t cbBytes, const byte_t* pData)
{
	try
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		// Check whether we are ready to send
		if (m_read_state != ready_to_send)
			OMEGA_THROW(EBUSY);

		// Prepare the message to send
		if (cbBytes && pData)
		{
			if (!m_mbRequest)
				OMEGA_NEW(m_mbRequest,ACE_Message_Block(cbBytes));
			else
				grow_mb(m_mbRequest,cbBytes);

			m_mbRequest->copy((const char*)pData,cbBytes);
		}

		// Clear the redirect loop set
		m_setRedirects.clear();

		Send_i();
	}
	catch (...)
	{
		ResetState();
		throw;
	}
}

void OOCore::HttpRequestAsync::Send_i()
{
	// Try to connect...
	if (!m_ptrStream)
	{
		m_ptrStream.Attach(IO::OpenStream(m_strAddr,this));
	}
	else
	{
		SendRequest();
	}
}

void OOCore::HttpRequestAsync::SendRequest()
{
	// Compose request
	uint32_t content_length = 0;
	if (m_mbRequest)
		content_length = (uint32_t)m_mbRequest->length();

	std::string strRequest = ComposeRequest(content_length).ToUTF8();

	// Reset the response buffer
	if (m_mbResponse)
	{
		if (m_mbResponse->cont())
		{
			m_mbResponse->cont()->release();
			m_mbResponse->cont(0);
		}
		m_mbResponse->reset();
	}

	// Copy the request
	size_t sz = strRequest.size();
	if (m_mbRequest)
	{
		ACE_Message_Block* mb = 0;
		OMEGA_NEW(mb,ACE_Message_Block(sz));
		mb->cont(m_mbRequest);
		m_mbRequest = mb;
	}
	else
		OMEGA_NEW(m_mbRequest,ACE_Message_Block(sz));

	m_mbRequest->copy(strRequest.c_str(),sz);

	// Set state
	m_read_state = sending_request;

	// Send the data
	m_ptrStream->WriteBytes(m_mbRequest->length(),(const byte_t*)m_mbRequest->rd_ptr());
}

void OOCore::HttpRequestAsync::ResetState()
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Close the stream
	m_ptrStream.Release();

	// Back to start
	m_read_state = closed;
}

void OOCore::HttpRequestAsync::OnOpened()
{
	try
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		SendRequest();
	}
	catch (IException* pE)
	{
		ResetState();

		ObjectPtr<IException> ptrE;
		ptrE.Attach(pE);
		m_ptrNotify->OnError(ptrE);
	}
}

void OOCore::HttpRequestAsync::OnError(IException* pE)
{
	m_ptrNotify->OnError(pE);

	ResetState();
}

void OOCore::HttpRequestAsync::OnWritten(const uint64_t& cbBytes)
{
	try
	{
		if (cbBytes > (size_t)-1)
			OMEGA_THROW(E2BIG);

		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		// Inc read ptr
		m_mbRequest->rd_ptr((size_t)cbBytes);

		// See if there is more to send
		if (m_mbRequest->length() == 0 && m_mbRequest->cont())
		{
			ACE_Message_Block* mb = m_mbRequest->cont();
			m_mbRequest->cont(0);
			m_mbRequest->release();
			m_mbRequest = mb;
		}

		if (m_mbRequest && m_mbRequest->length() > 0)
		{
			// Yes... send more
			m_ptrStream->WriteBytes(m_mbRequest->length(),(const byte_t*)m_mbRequest->rd_ptr());
		}
		else
		{
			// Reset the request buffer
			m_mbRequest->reset();

			// Prepare to read a response
			m_read_state = response_header;

			uint64_t cbRead = ACE_OS::getpagesize();
			m_ptrStream->ReadBytes(cbRead,0);
		}
	}
	catch (IException* pE)
	{
		ResetState();

		ObjectPtr<IException> ptrE;
		ptrE.Attach(pE);
		m_ptrNotify->OnError(ptrE);
	}
}

bool OOCore::HttpRequestAsync::ReadResponseHeader(unsigned int& notify_mask)
{
	// Try to parse the header...
	uint64_t cbRead = CheckHTTPHeader(m_mbBuffer);
	if (cbRead == (uint64_t)-1)
		OMEGA_THROW(L"Failed to parse HTTP header");

	if (cbRead != 0)
	{
		// Read some more
		m_ptrStream->ReadBytes(cbRead,0);
		return false;
	}

	// mb now contains a full response
	ParseResponseStatus(m_mbBuffer);
	SplitHTTPHeader(m_mbBuffer);

	// Check the status for redirects
	if (m_uStatus == 302 || m_uStatus == 307)
	{
		if (m_strMethod == L"GET" || m_strMethod == L"HEAD")
		{
			// Auto redirect
			string_t strLoc = HttpBase::GetResponseHeader(L"Location");

			// Open the new location
			bool bClose = open(m_strMethod,strLoc);
			if (!bClose)
			{
				// See if we have been told to close
				string_t strClose = HttpBase::GetResponseHeader(L"Connection");
				if (strClose == L"close")
					bClose = true;
			}

			if (bClose)
			{
				// Close the stream
				m_ptrStream.Release();
			}

			// Check for loops
			if (m_setRedirects.find(strLoc) != m_setRedirects.end())
				OMEGA_THROW(L"Redirection loop detected!");

			m_setRedirects.insert(strLoc);

			// Clear the buffer
			m_mbBuffer->reset();

			// Send the request again
			Send_i();

			// No more of this data
			return false;
		}
	}

	// Notify callback
	notify_mask |= 1;

	// Try to work out whether we are chunked or single block
	m_content_length = ACE_OS::getpagesize();
	string_t strC = HttpBase::GetResponseHeader(L"Content-Length");
	if (!strC.IsEmpty())
	{
		m_read_state = response_data;
		m_content_length = ACE_OS::strtoul(strC.c_str(),0,10);
	}
	else
	{
		strC = HttpBase::GetResponseHeader(L"Transfer-Encoding");
		if (strC.IsEmpty() || strC != L"chunked")
			OMEGA_THROW(L"No content length or chunking specified in response!");

		m_read_state = response_chunked;
	}

	return true;
}

void OOCore::HttpRequestAsync::ReadResponseData(unsigned int& notify_mask)
{
	// Single linear block... work out what's left to read
	m_content_length -= (uint32_t)m_mbBuffer->length();

	// Clone the buffer to the response
	if (!m_mbResponse)
		m_mbResponse = m_mbBuffer->clone();
	else
	{
		// Append to the cont() chain
		ACE_Message_Block* mb_cur = m_mbResponse;
		while (mb_cur->cont())
			mb_cur = mb_cur->cont();

		mb_cur->cont(m_mbBuffer->clone());
	}

	// Reset the buffer
	m_mbBuffer->reset();

	if (m_content_length)
	{
		// Notify the callback
		notify_mask |= 2;

		// Create new chunk
		uint64_t cbSize = ACE_OS::getpagesize();
		if (cbSize > m_content_length)
			cbSize = m_content_length;

		// Do a read
		m_ptrStream->ReadBytes(cbSize,0);
	}
	else
	{
		// Notify the callback
		notify_mask |= 4;

		// See if we have been told to close
		string_t strClose = HttpBase::GetResponseHeader(L"Connection");
		if (strClose == L"close")
		{
			// Close the stream
			m_ptrStream.Release();
		}

		// Back to a ready state
		m_read_state = ready_to_send;
	}
}

void OOCore::HttpRequestAsync::ReadResponseChunked(unsigned int& notify_mask)
{
	// Try to parse the chunking...
	ACE_Message_Block* mb = m_mbBuffer;
	uint64_t cbRead = ParseChunks(mb);
	if (cbRead == (uint64_t)-1)
		OMEGA_THROW(L"Failed to parse chunked encoding");

	// If we have parsed a block
	if (m_mbBuffer != mb)
	{
		ACE_Message_Block* mb_next = m_mbBuffer;
		while (mb_next->cont() != mb)
			mb_next = mb_next->cont();

		// Remove mb from chain
		mb_next->cont(0);

		// Append buffer to response
		if (!m_mbResponse)
			m_mbResponse = m_mbBuffer;
		else
		{
			mb_next = m_mbResponse;
			while (mb_next->cont())
				mb_next = mb_next->cont();

			mb_next->cont(m_mbBuffer);
		}

		// Buffer become mb
		m_mbBuffer = mb;
	}

	if (cbRead)
	{
		// Do a read
		m_ptrStream->ReadBytes(cbRead,0);

		if (m_mbResponse && m_mbResponse->total_length() > 0)
		{
			// Notify the callback
			notify_mask |= 2;
		}
	}
	else
	{
		// Notify the callback
		notify_mask |= 4;

		// See if we have been told to close
		string_t strClose = HttpBase::GetResponseHeader(L"Connection");
		if (strClose == L"close")
		{
			// Close the stream
			m_ptrStream.Release();
		}

		// Back to a ready state
		m_read_state = ready_to_send;
	}
}

void OOCore::HttpRequestAsync::OnRead(const uint64_t& cbBytes, const byte_t* pData)
{
	try
	{
		if (cbBytes > (size_t)-1)
			OMEGA_THROW(E2BIG);

		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		// Copy new data into the buffer
		if (!m_mbBuffer)
			OMEGA_NEW(m_mbBuffer,ACE_Message_Block((size_t)cbBytes));
		else
			grow_mb(m_mbBuffer,(size_t)cbBytes);

		m_mbBuffer->copy((const char*)pData,(size_t)cbBytes);

		// Now loop parsing
		unsigned int notify_mask = 0;
		for (bool bContinue = true;bContinue;)
		{
			bContinue = false;
			switch (m_read_state)
			{
			case response_header:
				bContinue = ReadResponseHeader(notify_mask);
				break;

			case response_data:
				ReadResponseData(notify_mask);
				break;

			case response_chunked:
				ReadResponseChunked(notify_mask);
				break;

			default:
				OMEGA_THROW(L"Invalid request state!");
			}
		}

		guard.release();

		if (notify_mask & 1)
			m_ptrNotify->OnResponseStart(m_uStatus,m_strStatusText);

		if (notify_mask & 2)
			m_ptrNotify->OnResponseDataAvailable();

		if (notify_mask & 4)
		{
			m_ptrNotify->OnResponseComplete();

			// Signal a completion event
			m_completion.signal();
		}
	}
	catch (IException* pE)
	{
		ResetState();

		ObjectPtr<IException> ptrE;
		ptrE.Attach(pE);
		m_ptrNotify->OnError(ptrE);
	}
}

uint16_t OOCore::HttpRequestAsync::Status()
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	return m_uStatus;
}

string_t OOCore::HttpRequestAsync::StatusText()
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	return m_strStatusText;
}

Omega::IEnumString* OOCore::HttpRequestAsync::GetAllResponseHeaders()
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	return HttpBase::GetAllResponseHeaders();
}

string_t OOCore::HttpRequestAsync::GetResponseHeader(const string_t& strHeader)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	return HttpBase::GetResponseHeader(strHeader);
}

void OOCore::HttpRequestAsync::ResponseBody(uint32_t& cbBytes, byte_t* pBody)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	if (!pBody)
	{
		if (m_mbResponse)
			cbBytes = (uint32_t)m_mbResponse->total_length();
		else
			cbBytes = 0;
		return;
	}

	if (!m_mbResponse)
		cbBytes = 0;
	else if (cbBytes > m_mbResponse->total_length())
		cbBytes = (uint32_t)m_mbResponse->total_length();

	ACE_Message_Block* pCur = m_mbResponse;
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

IO::IStream* OOCore::HttpRequestAsync::ResponseStream()
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	if (!m_mbResponse)
		return 0;

	ObjectPtr<ObjectImpl<HttpStream> > ptrStream = ObjectImpl<HttpStream>::CreateInstancePtr();
	ptrStream->init(&m_mbResponse,&m_lock);

	return ptrStream.AddRef();
}

void OOCore::HttpRequestAsync::SetRequestHeader(const string_t& strHeader, const string_t& strValue)
{
	HttpBase::SetRequestHeader(strHeader,strValue);
}

void OOCore::HttpRequestAsync::Abort()
{
	ResetState();

	// Notify the callback
	m_ptrNotify->OnResponseComplete();

	// Signal a completion event
	m_completion.signal();
}

bool_t OOCore::HttpRequestAsync::WaitForResponse(uint32_t timeout)
{
	OOBase::timeval_t spin(0,100);

	bool bRet = false;
	if (timeout)
	{
		OOBase::timeval_t val(timeout / 1000, (timeout % 1000) * 1000);
		OOBase::Countdown countdown(&val);

		do
		{
			bRet = (m_completion.wait(&spin,0) != -1);
			countdown.update();
		} while (!bRet && val != OOBase::timeval_t::zero && !Omega::HandleRequest(val.msec()));
	}
	else
	{
		do
		{
			bRet = (m_completion.wait(&spin,0) != -1);
		} while (!bRet && !Omega::HandleRequest(timeout));
	}

	return bRet;
}

void OOCore::HttpRequest::Open(const string_t& strMethod, const string_t& strURL, Net::Http::IRequestNotify* pAsyncNotify)
{
	if (m_ptrImpl)
	{
		// See if we need to switch from sync to async or vice-versa
		if ((m_bAsync && !pAsyncNotify) ||
			(!m_bAsync && pAsyncNotify))
		{
			m_ptrImpl.Release();
		}
	}

	if (!m_ptrImpl)
	{
		// Create the correct implementation
		if (pAsyncNotify)
		{
			ObjectPtr<ObjectImpl<OOCore::HttpRequestAsync> > ptrReq = ObjectImpl<OOCore::HttpRequestAsync>::CreateInstancePtr();
			m_ptrImpl.Attach(ptrReq.QueryInterface<Net::Http::IRequest>());
			m_bAsync = true;
		}
		else
		{
			ObjectPtr<ObjectImpl<OOCore::HttpRequestSync> > ptrReq = ObjectImpl<OOCore::HttpRequestSync>::CreateInstancePtr();
			m_ptrImpl.Attach(ptrReq.QueryInterface<Net::Http::IRequest>());
			m_bAsync = false;
		}
	}

	m_ptrImpl->Open(strMethod,strURL,pAsyncNotify);
}

void OOCore::HttpRequest::SetRequestHeader(const string_t& strHeader, const string_t& strValue)
{
	if (!m_ptrImpl)
		OMEGA_THROW(ENOTCONN);

	m_ptrImpl->SetRequestHeader(strHeader,strValue);
}

void OOCore::HttpRequest::Send(uint32_t cbBytes, const byte_t* pData)
{
	if (!m_ptrImpl)
		OMEGA_THROW(ENOTCONN);

	m_ptrImpl->Send(cbBytes,pData);
}

uint16_t OOCore::HttpRequest::Status()
{
	if (!m_ptrImpl)
		OMEGA_THROW(ENOTCONN);

	return m_ptrImpl->Status();
}

string_t OOCore::HttpRequest::StatusText()
{
	if (!m_ptrImpl)
		OMEGA_THROW(ENOTCONN);

	return m_ptrImpl->StatusText();
}

Omega::IEnumString* OOCore::HttpRequest::GetAllResponseHeaders()
{
	if (!m_ptrImpl)
		OMEGA_THROW(ENOTCONN);

	return m_ptrImpl->GetAllResponseHeaders();
}

string_t OOCore::HttpRequest::GetResponseHeader(const string_t& strHeader)
{
	if (!m_ptrImpl)
		OMEGA_THROW(ENOTCONN);

	return m_ptrImpl->GetResponseHeader(strHeader);
}

void OOCore::HttpRequest::ResponseBody(uint32_t& cbBytes, byte_t* pBody)
{
	if (!m_ptrImpl)
		OMEGA_THROW(ENOTCONN);

	m_ptrImpl->ResponseBody(cbBytes,pBody);
}

IO::IStream* OOCore::HttpRequest::ResponseStream()
{
	if (!m_ptrImpl)
		OMEGA_THROW(ENOTCONN);

	return m_ptrImpl->ResponseStream();
}

void OOCore::HttpRequest::Abort()
{
	if (!m_ptrImpl)
		OMEGA_THROW(ENOTCONN);

	m_ptrImpl->Abort();
}

bool_t OOCore::HttpRequest::WaitForResponse(uint32_t timeout)
{
	if (!m_ptrImpl)
		OMEGA_THROW(ENOTCONN);

	return m_ptrImpl->WaitForResponse(timeout);
}

/*OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_Net_Http_SplitURL,8,((in),const string_t&,strURL,(out),string_t&,strScheme,(out),string_t&,strHost,(out),string_t&,strPort,(out),string_t&,strUserName,(out),string_t&,strPassword,(out),string_t&,strResource,(out),string_t&,strQuery))
{
	// Default strResource to the URL
	strResource = strURL;

	// Find and remove scheme
	size_t pos = strResource.Find(L"://");
	if (pos != string_t::npos)
	{
		strScheme = strResource.Left(pos);
		strResource = strResource.Mid(pos + 3);
	}

	// Find and remove username:password
	pos = strResource.Find(L'@');
	if (pos != string_t::npos)
	{
		strUserName = strResource.Left(pos);
		strResource = strResource.Mid(pos + 1);

		pos = strUserName.Find(L':');
		if (pos != string_t::npos)
		{
			strPassword = strUserName.Mid(pos+1);
			strUserName = strUserName.Left(pos);
		}
	}

	// Find and remove hostname:port
	pos = strResource.Find(L'/');
	if (pos != string_t::npos)
	{
		strHost = strResource.Left(pos);
		strResource = strResource.Mid(pos);

		// Find resource?query
		pos = strResource.Find(L'?');
		if (pos != string_t::npos)
		{
			strQuery = strResource.Mid(pos + 1);
			strResource = strResource.Left(pos);
		}
	}
	else
	{
		strHost = strResource;
		strResource = L"/";
	}

	pos = strHost.Find(L':');
	if (pos != string_t::npos)
	{
		strPort = strHost.Mid(pos+1);
		strHost = strHost.Left(pos);
	}
}*/

OMEGA_DEFINE_OID(Omega::Net::Http,OID_StdHttpRequest,"{72B33743-6E02-49a1-89C3-C8B988492EF4}");
