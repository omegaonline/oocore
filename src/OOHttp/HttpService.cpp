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
#include "HttpService.h"

using namespace Omega;
using namespace OTL;

OMEGA_DEFINE_OID(OOHttp, OID_HttpService, "{8618D7FF-77A0-4785-B868-00BC81305FCE}");

void OOHttp::Service::Start(Omega::Registry::IKey* pKey)
{
	OTL::SingletonObjectImpl<Server>::CreateInstancePtr()->SetRegistryKey(pKey);
}

void OOHttp::Service::Stop()
{
}

void OOHttp::Service::OnAccept(Net::IAsyncSocket* pSocket)
{
	OTL::SingletonObjectImpl<Server>::CreateInstancePtr()->OnAccept(pSocket);	
}
