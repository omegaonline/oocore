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

namespace OOCore
{
	class WireStub;
	class WireProxy;

	// {3AC2D04F-A8C5-4214-AFE4-A64DB8DC992C}
	OMEGA_DECLARE_OID(OID_StdObjectManagerMarshalFactory);

	class StdObjectManagerMarshalFactory :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactorySingleton<StdObjectManagerMarshalFactory,&OID_StdObjectManagerMarshalFactory,Omega::Activation::InProcess>,
		public Omega::Remoting::IMarshalFactory
	{
	public:
		BEGIN_INTERFACE_MAP(StdObjectManagerMarshalFactory)
			INTERFACE_ENTRY(Omega::Remoting::IMarshalFactory)
		END_INTERFACE_MAP()

	// IMarshalFactory members
	public:
		void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject);
	};

	class StdObjectManager :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactoryNoAggregation<StdObjectManager,&Omega::Remoting::OID_StdObjectManager,Omega::Activation::InProcess>,
		public Omega::Remoting::IObjectManager,
		public Omega::System::MetaInfo::IWireManager,
		public Omega::System::MetaInfo::IWireManager_Safe,
		public Omega::Remoting::IMarshal,
		public Omega::System::MetaInfo::interface_info<Omega::Remoting::IMarshal>::safe_class
	{
	public:
		StdObjectManager();
		virtual ~StdObjectManager();

		BEGIN_INTERFACE_MAP(StdObjectManager)
			INTERFACE_ENTRY(Omega::Remoting::IObjectManager)
			INTERFACE_ENTRY(Omega::System::MetaInfo::IWireManager)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
		END_INTERFACE_MAP()

		void RemoveProxy(Omega::uint32_t proxy_id);
		void RemoveStub(Omega::uint32_t stub_id);
		bool IsAlive();

	private:
		StdObjectManager(const StdObjectManager&) :
            OTL::ObjectBase(),
            Omega::Remoting::IObjectManager(),
            Omega::System::MetaInfo::IWireManager(),
            Omega::System::MetaInfo::IWireManager_Safe(),
            Omega::Remoting::IMarshal(),
            Omega::System::MetaInfo::interface_info<Omega::Remoting::IMarshal>::safe_class()
        {}
		StdObjectManager& operator = (const StdObjectManager&) { return *this; };

		ACE_RW_Thread_Mutex                       m_lock;
		OTL::ObjectPtr<Omega::Remoting::IChannel> m_ptrChannel;
		Omega::uint32_t                           m_uNextStubId;
		
		std::map<Omega::System::MetaInfo::IObject_Safe*,WireStub*>                                     m_mapStubObjs;
		std::map<Omega::uint32_t,std::map<Omega::System::MetaInfo::IObject_Safe*,WireStub*>::iterator> m_mapStubIds;
		std::map<Omega::uint32_t,WireProxy*>                                                           m_mapProxyIds;

		void InvokeGetRemoteInstance(Omega::Remoting::IMessage* pParamsIn, OTL::ObjectPtr<Omega::Remoting::IMessage>& ptrResponse);
		
	// IObject_Safe members
	public:
		void OMEGA_CALL AddRef_Safe();
		void OMEGA_CALL Release_Safe();
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL QueryInterface_Safe(const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe** ppS);
		void OMEGA_CALL Pin() {}
		void OMEGA_CALL Unpin() {}

	// IWireManager members
	public:
		void MarshalInterface(const wchar_t* name, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::IObject* pObject);
		void ReleaseMarshalData(const wchar_t* name, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::IObject* pObject);
		void UnmarshalInterface(const wchar_t* name, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::IObject*& pObject);
		Omega::Remoting::IMessage* CreateMessage();
		Omega::IException* SendAndReceive(Omega::Remoting::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv, Omega::uint32_t timeout = 0);
		Omega::Remoting::IMessage* ReflectChannel();

	// IWireManager_Safe members
	public:
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL MarshalInterface_Safe(const wchar_t* name, Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe* pObject);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL ReleaseMarshalData_Safe(const wchar_t* name, Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe* pObject);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL UnmarshalInterface_Safe(const wchar_t* name, Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe** ppObject);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL CreateMessage_Safe(Omega::System::MetaInfo::IMessage_Safe** ppRet);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL SendAndReceive_Safe(Omega::System::MetaInfo::IException_Safe** ppRet, Omega::Remoting::MethodAttributes_t attribs, Omega::System::MetaInfo::IMessage_Safe* pSend, Omega::System::MetaInfo::IMessage_Safe** ppRecv, Omega::uint32_t timeout);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL ReflectChannel_Safe(Omega::System::MetaInfo::IMessage_Safe** ppRet);

	// IObjectManager members
	public:
		void Connect(Omega::Remoting::IChannel* pChannel);
		Omega::Remoting::IMessage* Invoke(Omega::Remoting::IMessage* pParamsIn, Omega::uint32_t timeout);
		void Disconnect();
		void GetRemoteInstance(const Omega::guid_t& oid, Omega::Activation::Flags_t flags, const Omega::guid_t& iid, Omega::IObject*& pObject);

	// IMarshal members
	public:
		Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		void MarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		void ReleaseMarshalData(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);

	// IMarshal_Safe members
	public:
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL GetUnmarshalFactoryOID_Safe(Omega::guid_t* pRet, const Omega::guid_t*, Omega::Remoting::MarshalFlags_t);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL MarshalInterface_Safe(Omega::System::MetaInfo::interface_info<Omega::Remoting::IObjectManager>::safe_class* pObjectManager, Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t* piid, Omega::Remoting::MarshalFlags_t flags);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL ReleaseMarshalData_Safe(Omega::System::MetaInfo::interface_info<Omega::Remoting::IObjectManager>::safe_class* pObjectManager, Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t* piid, Omega::Remoting::MarshalFlags_t flags);
	};
}

#endif // OOCORE_OBJECT_MANAGER_H_INCLUDED_
