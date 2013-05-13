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

#include "LocalTransport.h"

using namespace Omega;
using namespace OTL;

void OOCore::LocalTransport::init(OOBase::CDRStream& stream, OOBase::Proactor* proactor)
{

}

Remoting::IMessage* OOCore::LocalTransport::CreateMessage()
{
	return ObjectImpl<OOCore::CDRMessage>::CreateObject();
}

void OOCore::LocalTransport::SendMessage(Remoting::IMessage* pMessage)
{

}

string_t OOCore::LocalTransport::GetURI()
{
	return "local://pidfile";
}

uint32_t OOCore::LocalTransport::RegisterNotify(const guid_t& iid, IObject* pObject)
{
	uint32_t nCookie = 0;

	if (iid == OMEGA_GUIDOF(Remoting::ITransportNotify) && pObject)
	{
		ObjectPtr<Remoting::ITransportNotify> ptrNotify(static_cast<Remoting::ITransportNotify*>(pObject));
		ptrNotify.AddRef();

		OOBase::Guard<OOBase::SpinLock> guard(m_lock);

		int err = m_mapNotify.insert(ptrNotify,nCookie);
		if (err)
			OMEGA_THROW(err);
	}

	return nCookie;
}

void OOCore::LocalTransport::UnregisterNotify(uint32_t cookie)
{
	if (cookie)
	{
		OOBase::Guard<OOBase::SpinLock> guard(m_lock);

		m_mapNotify.remove(cookie);
	}
}

Notify::INotifier::iid_list_t OOCore::LocalTransport::ListNotifyInterfaces()
{
	Notify::INotifier::iid_list_t list;
	list.push_back(OMEGA_GUIDOF(Remoting::ITransportNotify));
	return list;
}

const guid_t OOCore::OID_LocalTransportMarshalFactory("{EEBD74BA-1C47-F582-BF49-92DFC17D83DE}");

void OOCore::LocalTransportMarshalFactory::UnmarshalInterface(Remoting::IMarshalContext* /*pMarshalContext*/, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t flags, IObject*& pObject)
{

}
