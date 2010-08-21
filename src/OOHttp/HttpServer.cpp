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
#include "RequestHandler.h"

using namespace Omega;
using namespace OTL;

void Http::HttpServer::SetRegistryKey(Omega::Registry::IKey* pKey)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	m_ptrKey = pKey;
}

void Http::HttpServer::OnAccept(Net::IAsyncSocket* pSocket)
{
	if (!pSocket)
		return;

	// Now to connect up a new request handler...
	ObjectPtr<ObjectImpl<RequestHandler> > ptrHandler = ObjectImpl<RequestHandler>::CreateInstancePtr();

	ptrHandler->Reset(pSocket);
}
