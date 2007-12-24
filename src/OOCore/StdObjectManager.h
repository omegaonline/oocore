///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
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
		public OTL::AutoObjectFactoryNoAggregation<StdObjectManagerMarshalFactory,&OID_StdObjectManagerMarshalFactory>,
		public Omega::Remoting::IMarshalFactory
	{
	public:
		BEGIN_INTERFACE_MAP(StdObjectManagerMarshalFactory)
			INTERFACE_ENTRY(Omega::Remoting::IMarshalFactory)
		END_INTERFACE_MAP()

	// IMarshalFactory members
	public:
		void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject);
	};

	class StdObjectManager :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactoryNoAggregation<StdObjectManager,&Omega::Remoting::OID_StdObjectManager>,
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

	private:
		StdObjectManager(const StdObjectManager&) : OTL::ObjectBase(),Omega::Remoting::IObjectManager(),Omega::System::MetaInfo::IWireManager() {};
		StdObjectManager& operator = (const StdObjectManager&) { return *this; };

		ACE_RW_Thread_Mutex                       m_lock;
		OTL::ObjectPtr<Omega::Remoting::IChannel> m_ptrChannel;
		Omega::uint32_t                           m_uNextStubId;
		Omega::Remoting::MarshalFlags_t           m_marshal_flags;

		std::map<Omega::System::MetaInfo::IObject_Safe*,WireStub*>   m_mapStubObjs;
		std::map<Omega::uint32_t,WireStub*>                          m_mapStubIds;
		std::map<Omega::uint32_t,WireProxy*>                         m_mapProxyIds;

	// IObject_Safe members
	public:
		void OMEGA_CALL AddRef_Safe();
		void OMEGA_CALL Release_Safe();
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL QueryInterface_Safe(const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe** ppS);
		void OMEGA_CALL Pin() {}
		void OMEGA_CALL Unpin() {}

	// IWireManager members
	public:
		void MarshalInterface(Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::IObject* pObject);
		void ReleaseMarshalData(Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::IObject* pObject);
		void UnmarshalInterface(Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::IObject*& pObject);
		Omega::Serialize::IFormattedStream* CreateOutputStream();
		Omega::IException* SendAndReceive(Omega::Remoting::MethodAttributes_t attribs, Omega::Serialize::IFormattedStream* pSend, Omega::Serialize::IFormattedStream*& pRecv, Omega::uint16_t timeout);

	// IWireManager_Safe members
	public:
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL MarshalInterface_Safe(Omega::System::MetaInfo::IFormattedStream_Safe* pStream, const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe* pObject);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL ReleaseMarshalData_Safe(Omega::System::MetaInfo::IFormattedStream_Safe* pStream, const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe* pObject);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL UnmarshalInterface_Safe(Omega::System::MetaInfo::IFormattedStream_Safe* pStream, const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe** ppObject);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL CreateOutputStream_Safe(Omega::System::MetaInfo::IFormattedStream_Safe** ppRet);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL SendAndReceive_Safe(Omega::System::MetaInfo::IException_Safe** ppRet, Omega::Remoting::MethodAttributes_t attribs, Omega::System::MetaInfo::IFormattedStream_Safe* pSend, Omega::System::MetaInfo::IFormattedStream_Safe** ppRecv, Omega::uint16_t timeout);

	// IObjectManager members
	public:
		void Connect(Omega::Remoting::IChannel* pChannel, Omega::Remoting::MarshalFlags_t marshal_flags);
		void Invoke(Omega::Serialize::IFormattedStream* pParamsIn, Omega::Serialize::IFormattedStream* pParamsOut);
		void Disconnect();
		void CreateRemoteInstance(const Omega::guid_t& oid, const Omega::guid_t& iid, Omega::IObject* pOuter, Omega::IObject*& pObject);

	// IMarshal members
	public:
		Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		void MarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		void ReleaseMarshalData(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);

	// IMarshal_Safe members
	public:
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL GetUnmarshalFactoryOID_Safe(Omega::guid_t* pRet, const Omega::guid_t*, Omega::Remoting::MarshalFlags_t);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL MarshalInterface_Safe(Omega::System::MetaInfo::interface_info<Omega::Remoting::IObjectManager>::safe_class* pObjectManager, Omega::System::MetaInfo::IFormattedStream_Safe* pStream, const Omega::guid_t* piid, Omega::Remoting::MarshalFlags_t flags);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL ReleaseMarshalData_Safe(Omega::System::MetaInfo::interface_info<Omega::Remoting::IObjectManager>::safe_class* pObjectManager, Omega::System::MetaInfo::IFormattedStream_Safe* pStream, const Omega::guid_t* piid, Omega::Remoting::MarshalFlags_t flags);
	};
}

#endif // OOCORE_OBJECT_MANAGER_H_INCLUDED_
