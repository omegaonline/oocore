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

#include "./CDRMessage.h"

namespace OOCore
{
	class Channel :
		public OTL::ObjectBase,
		public Omega::Remoting::IChannel,
		public Omega::Remoting::IMarshal
	{
	public:
		Channel();

		void init(ACE_CDR::ULong channel_id, Omega::Remoting::MarshalFlags_t marshal_flags, const Omega::guid_t& message_oid);
		void disconnect();

		BEGIN_INTERFACE_MAP(Channel)
			INTERFACE_ENTRY(Omega::Remoting::IChannelBase)
			INTERFACE_ENTRY(Omega::Remoting::IChannel)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
		END_INTERFACE_MAP()

	private:
		ACE_CDR::ULong	                                m_channel_id;
		Omega::Remoting::MarshalFlags_t                 m_marshal_flags;
		Omega::guid_t                                   m_message_oid;
		OTL::ObjectPtr<Omega::Remoting::IObjectManager> m_ptrOM;

		Channel(const Channel&) : OTL::ObjectBase(), Omega::Remoting::IChannel(), Omega::Remoting::IMarshal() {}
		Channel& operator = (const Channel&) { return *this; }

	// IChannelBase members
	public:
		Omega::Remoting::IMessage* CreateMessage();
		Omega::IException* SendAndReceive(Omega::Remoting::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv, Omega::uint32_t timeout);
		Omega::Remoting::MarshalFlags_t GetMarshalFlags();

		Omega::uint32_t GetSource();

	// IChannel members
	public:
		Omega::guid_t GetReflectUnmarshalFactoryOID();
		void ReflectMarshal(Omega::Remoting::IMessage* pMessage);
		Omega::Remoting::IObjectManager* GetObjectManager();

	// IMarshal members
	public:
		Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		void MarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		void ReleaseMarshalData(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
	};

	// {7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}
	OMEGA_DECLARE_OID(OID_ChannelMarshalFactory);

	class ChannelMarshalFactory :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactorySingleton<ChannelMarshalFactory,&OID_ChannelMarshalFactory,Omega::Activation::InProcess>,
		public Omega::Remoting::IMarshalFactory
	{
	public:
		BEGIN_INTERFACE_MAP(ChannelMarshalFactory)
			INTERFACE_ENTRY(Omega::Remoting::IMarshalFactory)
		END_INTERFACE_MAP()

	// IMarshalFactory members
	public:
		void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject);
	};

	class OutputCDRMarshalFactory :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactorySingleton<OutputCDRMarshalFactory,&OID_OutputCDRMarshalFactory,Omega::Activation::InProcess>,
		public Omega::Remoting::IMarshalFactory
	{
	public:
		BEGIN_INTERFACE_MAP(OutputCDRMarshalFactory)
			INTERFACE_ENTRY(Omega::Remoting::IMarshalFactory)
		END_INTERFACE_MAP()

	// IMarshalFactory members
	public:
		void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject);
	};
}

#endif // OOCORE_CHANNEL_H_INCLUDED_
