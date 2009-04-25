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

#ifndef OOSERVER_USER_HTTP_H_INCLUDED_
#define OOSERVER_USER_HTTP_H_INCLUDED_

#include <OONet/Http.h>

namespace User
{
	class Manager;

	class HttpConnection
	{
	public:
		HttpConnection(Manager* pManager, Omega::uint16_t conn_id, const ACE_CString& strRemoteAddr, const ACE_CString& strScheme);
		virtual ~HttpConnection();

		void Error();
		void Recv(const ACE_Message_Block* mb, ACE_CDR::ULong seq_no);
		void HTTPRespondError(const char* pszMessage, int err = 0, const char* pszHeaders = 0);
		Omega::string_t GetRequestHeader(const Omega::string_t& strHeader);
		void HTTPRedirect(Omega::string_t strResource, bool bRespond, const std::string& strHeaders = "");
		Omega::string_t BaseURI();

		struct Header
		{
			Omega::string_t strHeader;
			Omega::string_t strValue;
		};
		std::map<Omega::string_t,Header> m_mapRequestHeaders;
		Omega::string_t                  m_strMethod;
		Omega::string_t                  m_strResource;
		ACE_CString                      m_strScheme;
		ACE_Message_Block*               m_mbRecv;
		ACE_CDR::ULong                   m_seq_no;

		std::map<ACE_CDR::ULong,ACE_Message_Block*> m_mapOutOfSequence;
		
	private:
		OOBase::Mutex      m_lock;
		Manager*           m_pManager;
		Omega::uint16_t    m_conn_id;
		ACE_CString        m_strRemoteAddr;
		Omega::uint32_t    m_content_length;
		
		enum
		{
			request_header,
			request_data,
			request_chunked
		} m_read_state;

		std::string Trim(const std::string& str);
		const char* QuickFind(ACE_Message_Block* mb, const char* pStart, char c);
		bool ReadRequestHeader(const ACE_Message_Block* mb);
		void ReadRequestData(const ACE_Message_Block* mb);
		void ReadRequestChunked(const ACE_Message_Block* mb);
		void SplitHTTPHeader();
		bool SendToRoot(const ACE_Message_Block* mb);
		void Close();
	};

	class RequestStream : 
		public OTL::ObjectBase,
		public Omega::IO::IStream
	{
	public:
		RequestStream();
		virtual ~RequestStream();

		void init(const ACE_Message_Block* mb);

		BEGIN_INTERFACE_MAP(RequestStream)
			INTERFACE_ENTRY(Omega::IO::IStream)
		END_INTERFACE_MAP()

	private:
		OOBase::Mutex      m_lock;
		ACE_Message_Block* m_mb;
		
	public:
		void ReadBytes(Omega::uint64_t& cbBytes, Omega::byte_t* val);
		void WriteBytes(const Omega::uint64_t& cbBytes, const Omega::byte_t* val);
	};
}

#endif // OOSERVER_USER_HTTP_H_INCLUDED_
