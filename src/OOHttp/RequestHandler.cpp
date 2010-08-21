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
#include "RequestHandler.h"

using namespace Omega;
using namespace OTL;

void Http::RequestHandler::Reset(Net::IAsyncSocket* pSocket)
{
	OTL::ObjectPtr<Omega::Net::IAsyncSocket> ptrOld;

	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	if (m_ptrSocket)
	{
		ObjectPtr<Net::IAsyncSocketNotify> ptrN;
		ptrN.Attach(m_ptrSocket->Bind(0));
	}

	m_ptrSocket = pSocket;

	ObjectPtr<Net::IAsyncSocketNotify> ptrN;
	ptrN.Attach(m_ptrSocket->Bind(this));

	m_ptrSocket->Recv(1024,false);
}

void Http::RequestHandler::OnRecv(Omega::Net::IAsyncSocketBase* /*pSocket*/, Omega::uint32_t lenBytes, const Omega::byte_t* bytes, Omega::IException* pError)
{
	printf("%s",std::string((const char*)bytes,lenBytes).c_str());
}

void Http::RequestHandler::OnSent(Omega::Net::IAsyncSocketBase* /*pSocket*/, Omega::uint32_t lenBytes, const Omega::byte_t* bytes, Omega::IException* pError)
{
}

void Http::RequestHandler::OnClose(Omega::Net::IAsyncSocketBase* /*pSocket*/)
{
}
