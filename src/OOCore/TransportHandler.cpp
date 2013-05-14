///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2013 Rick Taylor
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

#include "OOCore_precomp.h"

#include "TransportHandler.h"

using namespace Omega;
using namespace OTL;

OOCore::TransportHandler::TransportHandler(UserSession* pSession) :
		m_pSession(pSession),
		m_notify_cookie(0)
{
}

void OOCore::TransportHandler::init(Remoting::ITransport* pTransport)
{
	ObjectPtr<Notify::INotifier> ptrNotify = OTL::QueryInterface<Notify::INotifier>(pTransport);
	if (!ptrNotify)
		throw OOCore_INotFoundException_MissingIID(OMEGA_GUIDOF(Notify::INotifier));

	m_notify_cookie = ptrNotify->RegisterNotify(OMEGA_GUIDOF(Remoting::ITransportNotify),this);
	if (!m_notify_cookie)
		throw OOCore_INotFoundException_MissingIID(OMEGA_GUIDOF(Remoting::ITransportNotify));

	m_ptrTransport = pTransport;
	m_ptrTransport.AddRef();
}

void OOCore::TransportHandler::close()
{
	// Unregister our notifier
	if (m_notify_cookie)
	{
		ObjectPtr<Notify::INotifier> ptrNotify = m_ptrTransport.QueryInterface<Notify::INotifier>();
		if (ptrNotify)
			ptrNotify->UnregisterNotify(m_notify_cookie);

		m_notify_cookie = 0;
	}

	m_ptrTransport.Release();
}

void OOCore::TransportHandler::OnMessage(Remoting::IMessage* pMessage)
{
	// Tell UserSession!
	void* TODO;
}

void OOCore::TransportHandler::OnClose()
{
	// Tell UserSession!
	void* TODO;
}
