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

#include "../../include/Omega/Remoting.h"

#include "WireProxy.h"
#include "WireStub.h"

namespace OOCore
{
	interface IStdObjectManager : public Omega::Remoting::IObjectManager
	{
		virtual void MarshalChannel(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, Omega::Remoting::MarshalFlags_t flags) = 0;
		virtual void ReleaseMarshalChannelData(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, Omega::Remoting::MarshalFlags_t flags) = 0;
	};

	Omega::TypeInfo::IInterfaceInfo* GetInterfaceInfo(const Omega::guid_t& iid);
}

OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL
(
	OOCore, IStdObjectManager, Omega::Remoting, IObjectManager, "{AC019AD3-1E57-4641-A584-772F9604E31D}",

	OMEGA_METHOD_VOID(MarshalChannel,3,((in),Omega::Remoting::IMarshaller*,pMarshaller,(in),Omega::Remoting::IMessage*,pMessage,(in),Omega::Remoting::MarshalFlags_t,flags))
	OMEGA_METHOD_VOID(ReleaseMarshalChannelData,3,((in),Omega::Remoting::IMarshaller*,pMarshaller,(in),Omega::Remoting::IMessage*,pMessage,(in),Omega::Remoting::MarshalFlags_t,flags))
)

namespace OOCore
{
	class StdObjectManager :
			public OTL::ObjectBase,
			public OTL::AutoObjectFactory<StdObjectManager,&Omega::Remoting::OID_StdObjectManager,Omega::Activation::ProcessScope>,
			public IStdObjectManager,
			public Omega::Remoting::IMarshaller
	{
	public:
		StdObjectManager();
		
		BEGIN_INTERFACE_MAP(StdObjectManager)
			INTERFACE_ENTRY(IStdObjectManager)
			INTERFACE_ENTRY(Omega::Remoting::IObjectManager)
			INTERFACE_ENTRY(Omega::Remoting::IMarshaller)
		END_INTERFACE_MAP()

		void RemoveProxy(Omega::uint32_t proxy_id);
		void RemoveStub(Omega::uint32_t stub_id);
		bool IsAlive();
		void DoMarshalChannel(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pParamsOut);
		void UndoMarshalChannel(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pParamsOut);

	private:
		StdObjectManager(const StdObjectManager&);
		StdObjectManager& operator = (const StdObjectManager&);

		OOBase::RWMutex                           m_lock;
		OTL::ObjectPtr<Omega::Remoting::IChannel> m_ptrChannel;
		
		OOBase::HashTable<Omega::IObject*,Omega::uint32_t>                           m_mapStubObjs;
		OOBase::HandleTable<Omega::uint32_t,OTL::ObjectPtr<OTL::ObjectImpl<Stub> > > m_mapStubIds;
		OOBase::HashTable<Omega::uint32_t,OTL::ObjectImpl<Proxy>*>                   m_mapProxyIds;

		void InvokeGetRemoteInstance(Omega::Remoting::IMessage* pParamsIn, Omega::Remoting::IMessage* pResponse);
		void InvokeGetInterfaceInfo(Omega::Remoting::IMessage* pParamsIn, Omega::Remoting::IMessage* pResponse);
		bool CustomMarshalInterface(Omega::Remoting::IMarshal* pMarshal, const Omega::guid_t& iid, Omega::Remoting::IMessage* pMessage);

	// IMarshaller members
	public:
		void MarshalInterface(const Omega::string_t& name, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::IObject* pObject);
		void ReleaseMarshalData(const Omega::string_t& name, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::IObject* pObject);
		void UnmarshalInterface(const Omega::string_t& name, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::IObject*& pObject);
		Omega::Remoting::IMessage* CreateMessage();
		Omega::IException* SendAndReceive(Omega::TypeInfo::MethodAttributes_t attribs, Omega::Remoting::IMessage* pSend, Omega::Remoting::IMessage*& pRecv, Omega::uint32_t timeout = 0);
		Omega::uint32_t GetSource();

	// IStdObjectManager members
	public:
		void MarshalChannel(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, Omega::Remoting::MarshalFlags_t flags);
		void ReleaseMarshalChannelData(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, Omega::Remoting::MarshalFlags_t flags);

	// IObjectManager members
	public:
		void Connect(Omega::Remoting::IChannel* pChannel);
		Omega::Remoting::IMessage* Invoke(Omega::Remoting::IMessage* pParamsIn, Omega::uint32_t timeout);
		void Shutdown();
		void GetRemoteInstance(const Omega::any_t& oid, Omega::Activation::Flags_t flags, const Omega::guid_t& iid, Omega::IObject*& pObject);
		Omega::TypeInfo::IInterfaceInfo* GetInterfaceInfo(const Omega::guid_t& iid);
	};
}

#endif // OOCORE_OBJECT_MANAGER_H_INCLUDED_
