///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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

#ifndef OOCORE_CHANNEL_H_INCLUDED_
#define OOCORE_CHANNEL_H_INCLUDED_

#include "CDRMessage.h"

namespace OOCore
{
	class UserSession;

	class ChannelBase :
			public OTL::ObjectBase,
			public Omega::Remoting::IChannel,
			public Omega::Remoting::IMarshal
	{
	public:
		ChannelBase();

		virtual void disconnect();

		Omega::Remoting::IObjectManager* GetObjectManager();

		BEGIN_INTERFACE_MAP(ChannelBase)
			INTERFACE_ENTRY(Omega::Remoting::IChannel)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
		END_INTERFACE_MAP()

	protected:
		void init(Omega::uint32_t channel_id, Omega::Remoting::MarshalFlags_t marshal_flags, Omega::Remoting::IObjectManager* pOM, const Omega::guid_t& message_oid);

		OOBase::SpinLock                                  m_lock;
		Omega::uint32_t                                   m_channel_id;
		Omega::Remoting::MarshalFlags_t                   m_marshal_flags;
		Omega::guid_t                                     m_message_oid;
		OTL::ObjectPtr<Omega::Remoting::IObjectManager>   m_ptrOM;
		OTL::ObjectPtr<Omega::Activation::IObjectFactory> m_ptrOF;

	private:
		ChannelBase(const ChannelBase&);
		ChannelBase& operator = (const ChannelBase&);

	// IChannel members
	public:
		virtual Omega::Remoting::IMessage* CreateMessage();
		virtual Omega::Remoting::MarshalFlags_t GetMarshalFlags();
		virtual Omega::uint32_t GetSource();
		virtual Omega::guid_t GetReflectUnmarshalFactoryOID();
		virtual void GetManager(const Omega::guid_t& iid, Omega::IObject*& pObject);
		virtual void ReflectMarshal(Omega::Remoting::IMessage* pMessage);

	// IMarshal members
	public:
		virtual Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		virtual void MarshalInterface(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		virtual void ReleaseMarshalData(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
	};

	class Channel :
			public ChannelBase
	{
	public:
		Channel() : m_pSession(0)
		{}

		void init(UserSession* pSession, Omega::uint32_t channel_id, Omega::Remoting::IObjectManager* pOM, const Omega::guid_t& message_oid);
		void shutdown(Omega::uint32_t closed_channel_id);
		void disconnect();

		BEGIN_INTERFACE_MAP(Channel)
			INTERFACE_ENTRY_CHAIN(ChannelBase)
		END_INTERFACE_MAP()

	private:
		UserSession*                                 m_pSession;
		OTL::ObjectPtr<Omega::Remoting::IMarshaller> m_ptrMarshaller;

	public:
		Omega::bool_t IsConnected();
		Omega::IException* SendAndReceive(Omega::TypeInfo::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv, Omega::uint32_t millisecs);
		void ReflectMarshal(Omega::Remoting::IMessage* pMessage);
	};

	// {7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}
	extern const Omega::guid_t OID_ChannelMarshalFactory;

	class ChannelMarshalFactory :
			public OTL::ObjectBase,
			public OTL::AutoObjectFactorySingleton<ChannelMarshalFactory,&OID_ChannelMarshalFactory,Omega::Activation::ProcessScope>,
			public Omega::Remoting::IMarshalFactory
	{
	public:
		BEGIN_INTERFACE_MAP(ChannelMarshalFactory)
			INTERFACE_ENTRY(Omega::Remoting::IMarshalFactory)
		END_INTERFACE_MAP()

	// IMarshalFactory members
	public:
		void UnmarshalInterface(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject);
	};

	class CDRMessageMarshalFactory :
			public OTL::ObjectBase,
			public OTL::AutoObjectFactorySingleton<CDRMessageMarshalFactory,&OID_CDRMessageMarshalFactory,Omega::Activation::ProcessScope>,
			public Omega::Remoting::IMarshalFactory
	{
	public:
		BEGIN_INTERFACE_MAP(CDRMessageMarshalFactory)
			INTERFACE_ENTRY(Omega::Remoting::IMarshalFactory)
		END_INTERFACE_MAP()

	// IMarshalFactory members
	public:
		void UnmarshalInterface(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject);
	};
}

#endif // OOCORE_CHANNEL_H_INCLUDED_
