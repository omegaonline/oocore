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

#ifndef OOHTTP_SERVER_H_INCLUDED_
#define OOHTTP_SERVER_H_INCLUDED_

#include "RequestHandler.h"

namespace OOHttp
{
	void GetStatusText(unsigned int status_code, unsigned long nVersion, std::string& strText, std::string& strExtended);

	class Server : 
			public OTL::ObjectBase
	{
	public:
		void InitOnce();

		void SetRegistryKey(Omega::Registry::IKey* pKey);
		void OnAccept(Omega::Net::IAsyncSocket* pSocket);
		void GetHeaders(std::map<std::string,std::string>& headers); 

		std::string GetErrorResponse(unsigned long nVersion, unsigned int status_code, const std::string strBodyText, const std::map<std::string,std::string>& headers = std::map<std::string,std::string>());
		OTL::ObjectPtr<Omega::Http::Server::IResource> FindResource(IRequestHandler* request, RequestHandler::Info& info);

		BEGIN_INTERFACE_MAP(Server)
		END_INTERFACE_MAP()

	private:
		Omega::Threading::Mutex               m_lock;
		OTL::ObjectPtr<Omega::Registry::IKey> m_ptrKey;
		
		std::string GetVersion() const;
		
		OTL::ObjectPtr<Omega::Http::Server::IResource> options_resource(IRequestHandler* request, const RequestHandler::Info& info);
		OTL::ObjectPtr<Omega::Http::Server::IResource> find_resource(IRequestHandler* request, const RequestHandler::Info& info);
	};
}

#endif // OOHTTP_SERVER_H_INCLUDED_
