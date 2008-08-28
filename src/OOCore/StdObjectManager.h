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
	class Stub;
	class Proxy;

	interface IStdObjectManager : public Omega::Remoting::IObjectManager
	{
		virtual void MarshalChannel(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, Omega::Remoting::MarshalFlags_t flags) = 0;
	};
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

		Omega::System::MetaInfo::IObject_Safe* CreateProxy(const Omega::guid_t& iid, Omega::System::MetaInfo::IProxy_Safe* pProxy);
		Omega::System::MetaInfo::IStub_Safe* CreateStub(const Omega::guid_t& iid, Omega::System::MetaInfo::IStubController_Safe* pController, Omega::System::MetaInfo::IObject_Safe* pObjS);

	private:
		StdObjectManager(const StdObjectManager&) :
            OTL::ObjectBase(),
            IStdObjectManager(),
            Omega::System::IMarshaller(),
            Omega::System::MetaInfo::IMarshaller_Safe()
        {}
		StdObjectManager& operator = (const StdObjectManager&) { return *this; };

		ACE_RW_Thread_Mutex                                  m_lock;
		OTL::ObjectPtr<Omega::Remoting::IChannel>            m_ptrChannel;
		Omega::uint32_t                                      m_uNextStubId;
		OTL::ObjectPtr<Omega::System::IProxyStubFactory> m_ptrPSFactory;
		
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
		Omega::IException* SendAndReceive(Omega::Remoting::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv, Omega::uint32_t timeout = 0);
		
	// IMarshaller_Safe members
	public:
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL MarshalInterface_Safe(const wchar_t* name, Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe* pObject);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL ReleaseMarshalData_Safe(const wchar_t* name, Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe* pObject);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL UnmarshalInterface_Safe(const wchar_t* name, Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe** ppObject);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL CreateMessage_Safe(Omega::System::MetaInfo::IMessage_Safe** ppRet);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL SendAndReceive_Safe(Omega::System::MetaInfo::IException_Safe** ppRet, Omega::Remoting::MethodAttributes_t attribs, Omega::System::MetaInfo::IMessage_Safe* pSend, Omega::System::MetaInfo::IMessage_Safe** ppRecv, Omega::uint32_t timeout);
		
	// IStdObjectManager members
	public:
		void MarshalChannel(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, Omega::Remoting::MarshalFlags_t flags);

	// IObjectManager members
	public:
		void Connect(Omega::Remoting::IChannelBase* pChannel);
		Omega::Remoting::IMessage* Invoke(Omega::Remoting::IMessage* pParamsIn, Omega::uint32_t timeout);
		void Disconnect();
		void SetProxyStubFactory(Omega::System::IProxyStubFactory* pPSFactory);
		void GetRemoteInstance(const Omega::string_t& strOID, Omega::Activation::Flags_t flags, const Omega::guid_t& iid, Omega::IObject*& pObject);
	};
}

#endif // OOCORE_OBJECT_MANAGER_H_INCLUDED_
