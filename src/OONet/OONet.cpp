///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OONet, the Omega Online Network library.
//
// OONet is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OONet is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OONet.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OONet_precomp.h"

#include "HttpImpl.h"

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

using namespace Omega;
using namespace OTL;

BEGIN_PROCESS_OBJECT_MAP()
	OBJECT_MAP_ENTRY(User::TcpProtocolHandler,0)
	OBJECT_MAP_ENTRY(User::HttpProtocolHandler,0)	
	OBJECT_MAP_ENTRY(OOCore::HttpRequest,L"Omega.Http.Request")
END_LIBRARY_OBJECT_MAP()



// HTTP handling (for sandbox only)
		OOBase::RWMutex                                                                      m_http_lock;
		std::map<Omega::uint16_t,OOBase::SmartPtr<HttpConnection> >                          m_mapHttpConnections;
		std::map<Omega::string_t,OTL::ObjectPtr<Omega::Net::Http::Server::IRequestHandler> > m_mapHttpHandlers;
		
		void open_http(ACE_InputCDR& request, ACE_OutputCDR& response);
		void recv_http(ACE_InputCDR& request);
		void handle_http_request_i(HttpConnection* pConn, Omega::uint16_t conn_id);