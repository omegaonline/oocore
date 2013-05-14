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

#ifndef OOCORE_TRANSPORT_HANDLER_H_INCLUDED_
#define OOCORE_TRANSPORT_HANDLER_H_INCLUDED_

#include "UserSession.h"

namespace OOCore
{
	class TransportHandler :
			public OTL::ObjectBase,
			public OOBase::NonCopyable,
			public Omega::Remoting::ITransportNotify
	{
	public:
		TransportHandler(UserSession* pSession);

		void init(Omega::Remoting::ITransport* pTransport);
		void close();

		BEGIN_INTERFACE_MAP(TransportHandler)
			INTERFACE_ENTRY(Omega::Remoting::ITransportNotify)
		END_INTERFACE_MAP()

	private:
		UserSession*                                m_pSession;
		Omega::uint32_t                             m_notify_cookie;
		OTL::ObjectPtr<Omega::Remoting::ITransport> m_ptrTransport;

	public:
		virtual void OnMessage(Omega::Remoting::IMessage* pMessage);
		virtual void OnClose();
	};
}

#endif // OOCORE_TRANSPORT_HANDLER_H_INCLUDED_
