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

#ifndef OOHTTP_REQUEST_HANDLER_H_INCLUDED_
#define OOHTTP_REQUEST_HANDLER_H_INCLUDED_

#include "QuickBuffer.h"

namespace OOHttp
{
	class Server;

	interface IRequestHandler : public Omega::IObject
	{
	public:
		virtual void SendHeader(Omega::uint16_t status_code, const Omega::Http::Server::IRequest::header_map_t& mapHeaders) = 0;
		virtual void Send(Omega::uint32_t lenBytes, const Omega::byte_t* data) = 0;
	};

	class RequestHandler :
		public OTL::ObjectBase,
		public Omega::Net::IAsyncSocketNotify,
		public IRequestHandler
	{
	public:
		RequestHandler() {}
		
		void Init(Server* pServer);
		void Reset(Omega::Net::IAsyncSocket* pSocket);
				
		enum Method
		{
			mOptions,
			mGet,
			mHead,
			mPost,
			mPut,
			mDelete,
			mTrace,
			mConnect,
			mUnknown
		};

		struct Info
		{
			unsigned long                     m_uVersion;
			Method                            m_method;
			std::string                       m_strMethod;
			std::string                       m_strHost;
			std::string                       m_strResource;
			std::map<std::string,std::string> m_mapHeaders;
			size_t                            m_ulContent;
		};

		BEGIN_INTERFACE_MAP(RequestHandler)
			INTERFACE_ENTRY(Omega::Net::IAsyncSocketNotify)
		END_INTERFACE_MAP()

	private:
		Omega::Threading::Mutex                        m_lock;
		OTL::ObjectPtr<Omega::Net::IAsyncSocket>       m_ptrSocket;
		QuickBuffer                                    m_buffer;		
		Server*                                        m_pServer;
		OTL::ObjectPtr<Omega::Http::Server::IResource> m_ptrResource;

		enum ParseState
		{
			parseRequest,
			parseHeaders,
			parseBody,
			recvBody,
			recvChunked,
			recvComplete
		};
		ParseState m_parseState;
		Info       m_info;
		size_t     m_ulContentRead;

		void handle_result(int result);
		int parse_request_line();
		int parse_headers();
		int parse_field(const char*& rd_ptr, const char* end_ptr, std::string& strKey, std::string& strValue);
		int parse_body();
		bool skip_lws(const char*& rd_ptr, const char* end_ptr);
		int recv_body();
		int recv_chunked();
		void recv_complete();
		int report_error(unsigned int status_code, const std::string& strBodyText = std::string());
		void close();

		void SendHeader(Omega::uint16_t status_code, const Omega::Http::Server::IRequest::header_map_t& mapHeaders);
		void Send(Omega::uint32_t lenBytes, const Omega::byte_t* data);

	// Net::IAsyncSocketNotify members
	public:
		void OnRecv(Omega::Net::IAsyncSocketBase* pSocket, Omega::uint32_t lenBytes, const Omega::byte_t* bytes, Omega::IException* pError);
		void OnSent(Omega::Net::IAsyncSocketBase* pSocket, Omega::uint32_t lenBytes, const Omega::byte_t* bytes, Omega::IException* pError);
		void OnClose(Omega::Net::IAsyncSocketBase* pSocket);
	};
}

#endif // OOHTTP_REQUEST_HANDLER_H_INCLUDED_
