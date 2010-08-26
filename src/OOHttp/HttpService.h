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

#ifndef OOHTTP_SERVICE_H_INCLUDED_
#define OOHTTP_SERVICE_H_INCLUDED_

#include "HttpServer.h"

namespace OOHttp
{
	extern "C" const Omega::guid_t OID_HttpService;
	
	class Service :
			public OTL::ObjectBase,
			public OTL::AutoObjectFactory<Service,&OID_HttpService,Omega::Activation::MachineLocal | Omega::Activation::SingleUse>,
			public Omega::System::INetworkService
	{
	public:
		BEGIN_INTERFACE_MAP(Service)
			INTERFACE_ENTRY(Omega::System::INetworkService)
			INTERFACE_ENTRY(Omega::System::IService)
		END_INTERFACE_MAP()

	// IService members
	public:
		virtual void Start(Omega::Registry::IKey* pKey);
		virtual void Stop();

	// INetworkServce members
	public:
		virtual void OnAccept(Omega::Net::IAsyncSocket* pSocket);
	};
}

#endif // OOHTTP_SERVICE_H_INCLUDED_
