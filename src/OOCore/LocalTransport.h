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

#ifndef OOCORE_LOCAL_TRANSPORT_H_INCLUDED_
#define OOCORE_LOCAL_TRANSPORT_H_INCLUDED_

#include "Channel.h"

namespace OOCore
{
	class LocalTransport :
			public OTL::ObjectBase,
			public OOBase::NonCopyable,
			public Omega::Remoting::ITransport,
			public Omega::Notify::INotifier
	{
	public:
		LocalTransport() {}

		void init(OOBase::CDRStream& stream, OOBase::Proactor* proactor);

		BEGIN_INTERFACE_MAP(LocalTransport)
			INTERFACE_ENTRY(Omega::Remoting::ITransport)
		END_INTERFACE_MAP()

	private:
		OOBase::SpinLock m_lock;

		OOBase::HandleTable<Omega::uint32_t,OTL::ObjectPtr<Omega::Remoting::ITransportNotify> > m_mapNotify;

	// ITransport members
	public:
		Omega::Remoting::IMessage* CreateMessage();
		void SendMessage(Omega::Remoting::IMessage* pMessage);
		Omega::string_t GetURI();

	// INotifier members
	public:
		Omega::uint32_t RegisterNotify(const Omega::guid_t& iid, Omega::IObject* pObject);
		void UnregisterNotify(Omega::uint32_t cookie);
		iid_list_t ListNotifyInterfaces();
	};

	// {EEBD74BA-1C47-F582-BF49-92DFC17D83DE}
	extern const Omega::guid_t OID_LocalTransportMarshalFactory;

	class LocalTransportMarshalFactory :
			public OTL::ObjectBase,
			public OTL::AutoObjectFactorySingleton<LocalTransportMarshalFactory,&OID_LocalTransportMarshalFactory,Omega::Activation::ProcessScope>,
			public Omega::Remoting::IMarshalFactory
	{
	public:
		BEGIN_INTERFACE_MAP(LocalTransportMarshalFactory)
			INTERFACE_ENTRY(Omega::Remoting::IMarshalFactory)
		END_INTERFACE_MAP()

	// IMarshalFactory members
	public:
		void UnmarshalInterface(Omega::Remoting::IMarshalContext* pMarshalContext, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject);
	};
}

#endif // OOCORE_LOCAL_TRANSPORT_H_INCLUDED_
