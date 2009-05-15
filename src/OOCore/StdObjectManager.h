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

#ifndef OOCORE_OBJECT_MANAGER_H_INCLUDED_
#define OOCORE_OBJECT_MANAGER_H_INCLUDED_

#include <OOCore/Remoting.h>

namespace OOCore
{
	class Stub;
	class Proxy;

	interface IStdObjectManager : public Omega::Remoting::IObjectManager
	{
		virtual void MarshalChannel(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, Omega::Remoting::MarshalFlags_t flags) = 0;
	};

	// Some helpers
	inline Omega::bool_t ReadBoolean(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::bool_t val;
		if (pMsg->ReadBooleans(name,1,&val) != 1)
			OMEGA_THROW(L"Unexpected end of message");
		return val;
	}

	inline Omega::byte_t ReadByte(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::byte_t val;
		if (pMsg->ReadBytes(name,1,&val) != 1)
			OMEGA_THROW(L"Unexpected end of message");
		return val;
	}

	inline Omega::uint16_t ReadUInt16(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::uint16_t val;
		if (pMsg->ReadUInt16s(name,1,&val) != 1)
			OMEGA_THROW(L"Unexpected end of message");
		return val;
	}

	inline Omega::uint32_t ReadUInt32(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::uint32_t val;
		if (pMsg->ReadUInt32s(name,1,&val) != 1)
			OMEGA_THROW(L"Unexpected end of message");
		return val;
	}

	inline Omega::guid_t ReadGuid(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::guid_t val;
		if (pMsg->ReadGuids(name,1,&val) != 1)
			OMEGA_THROW(L"Unexpected end of message");
		return val;
	}

	inline void WriteBoolean(const wchar_t* name, Omega::Remoting::IMessage* pMsg, Omega::bool_t val)
	{
		pMsg->WriteBooleans(name,1,&val);
	}

	inline void WriteByte(const wchar_t* name, Omega::Remoting::IMessage* pMsg, Omega::byte_t val)
	{
		pMsg->WriteBytes(name,1,&val);
	}

	inline void WriteUInt16(const wchar_t* name, Omega::Remoting::IMessage* pMsg, Omega::uint16_t val)
	{
		pMsg->WriteUInt16s(name,1,&val);
	}

	inline void WriteUInt32(const wchar_t* name, Omega::Remoting::IMessage* pMsg, Omega::uint32_t val)
	{
		pMsg->WriteUInt32s(name,1,&val);
	}

	inline void WriteGuid(const wchar_t* name, Omega::Remoting::IMessage* pMsg, Omega::guid_t val)
	{
		pMsg->WriteGuids(name,1,&val);
	}

	Omega::TypeInfo::ITypeInfo* GetTypeInfo(const Omega::guid_t& iid);
}

OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL
(
	OOCore, IStdObjectManager, Omega::Remoting, IObjectManager, "{AC019AD3-1E57-4641-A584-772F9604E31D}",

	OMEGA_METHOD_VOID(MarshalChannel,3,((in),Omega::Remoting::IObjectManager*,pObjectManager,(in),Omega::Remoting::IMessage*,pMessage,(in),Omega::Remoting::MarshalFlags_t,flags))
)

namespace OOCore
{
	class StdObjectManager :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactoryNoAggregation<StdObjectManager,&Omega::Remoting::OID_StdObjectManager,Omega::Activation::InProcess>,
		public IStdObjectManager,
		public Omega::System::IMarshaller,
		public Omega::System::MetaInfo::IMarshaller_Safe
	{
	public:
		StdObjectManager();
		virtual ~StdObjectManager();

		BEGIN_INTERFACE_MAP(StdObjectManager)
			INTERFACE_ENTRY(IStdObjectManager)
			INTERFACE_ENTRY(Omega::Remoting::IObjectManager)
			INTERFACE_ENTRY(Omega::System::IMarshaller)
		END_INTERFACE_MAP()

		void RemoveProxy(Omega::uint32_t proxy_id);
		void RemoveStub(Omega::uint32_t stub_id);
		bool IsAlive();
		void DoMarshalChannel(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pParamsOut);

	private:
		StdObjectManager(const StdObjectManager&) :
            OTL::ObjectBase(),
            IStdObjectManager(),
            Omega::System::IMarshaller(),
            Omega::System::MetaInfo::IMarshaller_Safe()
        {}
		StdObjectManager& operator = (const StdObjectManager&) { return *this; };

		OOBase::RWMutex                           m_lock;
		OTL::ObjectPtr<Omega::Remoting::IChannel> m_ptrChannel;
		Omega::uint32_t                           m_uNextStubId;
				
		std::map<Omega::System::MetaInfo::IObject_Safe*,Stub*>                                     m_mapStubObjs;
		std::map<Omega::uint32_t,std::map<Omega::System::MetaInfo::IObject_Safe*,Stub*>::iterator> m_mapStubIds;
		std::map<Omega::uint32_t,Proxy*>                                                           m_mapProxyIds;

		void InvokeGetRemoteInstance(Omega::Remoting::IMessage* pParamsIn, OTL::ObjectPtr<Omega::Remoting::IMessage>& ptrResponse);
		
	// IObject_Safe members
	public:
		void OMEGA_CALL AddRef_Safe();
		void OMEGA_CALL Release_Safe();
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL QueryInterface_Safe(const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe** ppS);
		void OMEGA_CALL Pin() {}
		void OMEGA_CALL Unpin() {}

	// IMarshaller members
	public:
		void MarshalInterface(const wchar_t* name, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::IObject* pObject);
		void ReleaseMarshalData(const wchar_t* name, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::IObject* pObject);
		void UnmarshalInterface(const wchar_t* name, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::IObject*& pObject);
		Omega::Remoting::IMessage* CreateMessage();
		Omega::IException* SendAndReceive(Omega::TypeInfo::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv, Omega::uint32_t timeout = 0);
		Omega::TypeInfo::ITypeInfo* GetTypeInfo(const Omega::guid_t& iid);
		
	// IMarshaller_Safe members
	public:
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL MarshalInterface_Safe(const wchar_t* name, Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe* pObject);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL ReleaseMarshalData_Safe(const wchar_t* name, Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe* pObject);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL UnmarshalInterface_Safe(const wchar_t* name, Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe** ppObject);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL CreateMessage_Safe(Omega::System::MetaInfo::IMessage_Safe** ppRet);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL SendAndReceive_Safe(Omega::System::MetaInfo::IException_Safe** ppRet, Omega::TypeInfo::MethodAttributes_t attribs, Omega::System::MetaInfo::IMessage_Safe* pSend, Omega::System::MetaInfo::IMessage_Safe** ppRecv, Omega::uint32_t timeout);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL GetTypeInfo_Safe(Omega::System::MetaInfo::ITypeInfo_Safe** ppTypeInfo, const Omega::guid_t* piid);
		
	// IStdObjectManager members
	public:
		void MarshalChannel(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, Omega::Remoting::MarshalFlags_t flags);

	// IObjectManager members
	public:
		void Connect(Omega::Remoting::IChannelBase* pChannel);
		Omega::Remoting::IMessage* Invoke(Omega::Remoting::IMessage* pParamsIn, Omega::uint32_t timeout);
		void Shutdown();
		void GetRemoteInstance(const Omega::string_t& strOID, Omega::Activation::Flags_t flags, const Omega::guid_t& iid, Omega::IObject*& pObject);
	};
}

#endif // OOCORE_OBJECT_MANAGER_H_INCLUDED_
