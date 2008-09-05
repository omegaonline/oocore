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
	class UserSession;

	class Channel :
		public OTL::ObjectBase,
		public Omega::Remoting::IChannel,
		public Omega::Remoting::IMarshal
	{
	public:
		Channel();
		virtual ~Channel() {}

		void init(UserSession* pSession, ACE_CDR::UShort apt_id, ACE_CDR::ULong channel_id, Omega::Remoting::MarshalFlags_t marshal_flags, const Omega::guid_t& message_oid, Omega::Remoting::IObjectManager* pOM);
		void disconnect();

		BEGIN_INTERFACE_MAP(Channel)
			INTERFACE_ENTRY(Omega::Remoting::IChannelBase)
			INTERFACE_ENTRY(Omega::Remoting::IChannel)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
		END_INTERFACE_MAP()

	protected:
		ACE_CDR::UShort                                   m_apt_id;

	private:
		UserSession*                                      m_pSession;
		ACE_CDR::ULong	                                  m_channel_id;
		Omega::Remoting::MarshalFlags_t                   m_marshal_flags;
		Omega::guid_t                                     m_message_oid;
		OTL::ObjectPtr<Omega::Remoting::IObjectManager>   m_ptrOM;
		OTL::ObjectPtr<Omega::Activation::IObjectFactory> m_ptrOF;

		Channel(const Channel&) : OTL::ObjectBase(), Omega::Remoting::IChannel(), Omega::Remoting::IMarshal() {}
		Channel& operator = (const Channel&) { return *this; }

	// IChannelBase members
	public:
		virtual Omega::Remoting::IMessage* CreateMessage();
		virtual Omega::IException* SendAndReceive(Omega::TypeInfo::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv, Omega::uint32_t timeout);
		virtual Omega::Remoting::MarshalFlags_t GetMarshalFlags();
		virtual Omega::uint32_t GetSource();

	// IChannel members
	public:
		virtual Omega::guid_t GetReflectUnmarshalFactoryOID();
		virtual void ReflectMarshal(Omega::Remoting::IMessage* pMessage);
		virtual Omega::Remoting::IObjectManager* GetObjectManager();

	// IMarshal members
	public:
		virtual Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		virtual void MarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		virtual void ReleaseMarshalData(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
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
