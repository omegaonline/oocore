///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOSERVER_CHANNEL_H_INCLUDED_
#define OOSERVER_CHANNEL_H_INCLUDED_

#include "../OOCore/CDRMessage.h"

namespace User
{
	class Manager;

	class Channel :
			public OTL::ObjectBase,
			public Omega::Remoting::IChannel,
			public Omega::Remoting::IMarshal
	{
	public:
		Channel();

		void init(Manager* pManager, Omega::uint32_t channel_id, Omega::Remoting::MarshalFlags_t marshal_flags, const Omega::guid_t& message_oid);
		void disconnect();

		OTL::ObjectPtr<Omega::Remoting::IObjectManager> GetObjectManager()
		{
			return m_ptrOM;
		}

		BEGIN_INTERFACE_MAP(Channel)
			INTERFACE_ENTRY(Omega::Remoting::IChannel)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
		END_INTERFACE_MAP()

	private:
		OOBase::SpinLock                                  m_lock;
		Manager*                                          m_pManager;
		Omega::uint32_t                                   m_channel_id;
		Omega::Remoting::MarshalFlags_t                   m_marshal_flags;
		Omega::guid_t                                     m_message_oid;
		OTL::ObjectPtr<Omega::Remoting::IObjectManager>   m_ptrOM;
		OTL::ObjectPtr<Omega::Activation::IObjectFactory> m_ptrOF;
		OTL::ObjectPtr<Omega::Remoting::IMarshaller>      m_ptrMarshaller;

		Channel(const Channel&);
		Channel& operator = (const Channel&);

	// IChannel members
	public:
		Omega::Remoting::IMessage* CreateMessage();
		Omega::IException* SendAndReceive(Omega::TypeInfo::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv, Omega::uint32_t timeout);
		Omega::Remoting::MarshalFlags_t GetMarshalFlags();
		Omega::uint32_t GetSource();
		Omega::bool_t IsConnected();
		Omega::guid_t GetReflectUnmarshalFactoryOID();
		void ReflectMarshal(Omega::Remoting::IMessage* pMessage);
		void GetManager(const Omega::guid_t& iid, Omega::IObject*& pObject);

	// IMarshal members
	public:
		Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		void MarshalInterface(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		void ReleaseMarshalData(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
	};

	// {1A7672C5-8478-4e5a-9D8B-D5D019E25D15}
	extern "C" const Omega::guid_t OID_ChannelMarshalFactory;

	class ChannelMarshalFactory :
			public OTL::ObjectBase,
			public OTL::AutoObjectFactory<ChannelMarshalFactory,&OID_ChannelMarshalFactory,Omega::Activation::ProcessScope>,
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
}

#endif // OOSERVER_CHANNEL_H_INCLUDED_
