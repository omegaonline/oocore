///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
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

#ifndef OOCORE_APARTMENT_IMPL_H_INCLUDED_
#define OOCORE_APARTMENT_IMPL_H_INCLUDED_

namespace OOCore
{
	struct Message;
	class UserSession;
	class AptChannel;

	class Apartment
	{
	public:
		Apartment(UserSession* pSession, ACE_CDR::UShort id);
		
		void close();
		
		void process_channel_close(ACE_CDR::ULong closed_channel_id);
		bool is_channel_open(ACE_CDR::ULong channel_id);

		OTL::ObjectPtr<Omega::Remoting::IObjectManager> get_channel_om(ACE_CDR::ULong src_channel_id);
		OTL::ObjectPtr<OTL::ObjectImpl<Channel> > create_channel(ACE_CDR::ULong src_channel_id, const Omega::guid_t& message_oid);
		void process_request(const Message* pMsg, const ACE_Time_Value& deadline);

		OTL::ObjectPtr<Omega::Remoting::IObjectManager> get_apartment_om(ACE_CDR::UShort apartment_id);
		Omega::IException* apartment_message(ACE_CDR::UShort apt_id, Omega::TypeInfo::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv, Omega::uint32_t timeout);
		
	private:
		ACE_RW_Thread_Mutex m_lock;
		UserSession*        m_pSession;
		ACE_CDR::UShort     m_id;

		std::map<ACE_CDR::ULong,OTL::ObjectPtr<OTL::ObjectImpl<Channel> > >     m_mapChannels;
		std::map<ACE_CDR::UShort,OTL::ObjectPtr<OTL::ObjectImpl<AptChannel> > > m_mapApartments;
	};

	class AptChannel :
		public Channel
	{
	public:
		AptChannel() : Channel() {}

		void init(ACE_CDR::UShort apt_id, ACE_Refcounted_Auto_Ptr<Apartment,ACE_Thread_Mutex> ptrApt, Omega::Remoting::IObjectManager* pOM);
		//void init(ACE_CDR::ULong channel_id, Omega::Remoting::MarshalFlags_t marshal_flags, const Omega::guid_t& message_oid);
		//void disconnect();

		BEGIN_INTERFACE_MAP(AptChannel)
			INTERFACE_ENTRY_CHAIN(Channel)
		END_INTERFACE_MAP()

	private:
		AptChannel(const AptChannel&) : Channel() {}
		AptChannel& operator = (const AptChannel&) { return *this; }

		ACE_Refcounted_Auto_Ptr<Apartment,ACE_Thread_Mutex> m_ptrApt;

	// IChannelBase members
	public:
		virtual Omega::IException* SendAndReceive(Omega::TypeInfo::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv, Omega::uint32_t timeout);
	
	//// IChannel members
	//public:
	//	virtual Omega::guid_t GetReflectUnmarshalFactoryOID();
	//	virtual void ReflectMarshal(Omega::Remoting::IMessage* pMessage);
	
	//// IMarshal members
	//public:
	//	virtual Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
	//	virtual void MarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
	//	virtual void ReleaseMarshalData(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
	};

	// {6654B003-44F1-497a-B539-80B5FCED73BC}
	OMEGA_DECLARE_OID(OID_StdApartment);

	class ApartmentImpl :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactoryNoAggregation<ApartmentImpl,&OOCore::OID_StdApartment,Omega::Activation::InProcess>,
		public Omega::Apartment::IApartment
	{
	public:
		ApartmentImpl();
		virtual ~ApartmentImpl();

		BEGIN_INTERFACE_MAP(ApartmentImpl)
			INTERFACE_ENTRY(Omega::Apartment::IApartment)
		END_INTERFACE_MAP()

	private:
		ACE_CDR::UShort m_id;

	// IApartment members
	public:
		void CreateInstance(const Omega::string_t& oid, Omega::Activation::Flags_t flags, Omega::IObject* pOuter, const Omega::guid_t& iid, Omega::IObject*& pObject);
	};
}

#endif // OOCORE_APARTMENT_IMPL_H_INCLUDED_
