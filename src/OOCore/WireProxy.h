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

#ifndef OOCORE_WIREPROXY_H_INCLUDED_
#define OOCORE_WIREPROXY_H_INCLUDED_

#include "StdObjectManager.h"

namespace OOCore
{
	// {69099DD8-A628-458a-861F-009E016DB81B}
	OMEGA_DECLARE_OID(OID_ProxyMarshalFactory);

	class ProxyMarshalFactory :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactorySingleton<ProxyMarshalFactory,&OID_ProxyMarshalFactory,Omega::Activation::InProcess>,
		public Omega::Remoting::IMarshalFactory
	{
	public:
		BEGIN_INTERFACE_MAP(ProxyMarshalFactory)
			INTERFACE_ENTRY(Omega::Remoting::IMarshalFactory)
		END_INTERFACE_MAP()

	// IMarshalFactory members
	public:
		void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject);
	};

	class Proxy :
		public Omega::System::MetaInfo::IProxy_Safe,
		public Omega::System::MetaInfo::interface_info<Omega::Remoting::IMarshal>::safe_class
	{
	public:
		Proxy(Omega::uint32_t proxy_id, StdObjectManager* pManager);
		virtual ~Proxy();

		void Disconnect();

		Omega::System::MetaInfo::IObject_Safe* UnmarshalInterface(Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t& iid);

	// IObject_Safe methods
	public:
		void OMEGA_CALL AddRef_Safe()
		{
			++m_refcount;
		}

		void OMEGA_CALL Release_Safe()
		{
			if (--m_refcount==0)
				delete this;
		}

		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL QueryInterface_Safe(const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe** ppS);

		void OMEGA_CALL Pin()
		{
			// This needs implementing
			void* TICKET_88;
		}

		void OMEGA_CALL Unpin()
		{
			// This needs implementing
			void* TICKET_88;
		}

	// IProxy_Safe members
	public:
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL WriteKey_Safe(Omega::System::MetaInfo::IMessage_Safe* pMessage)
		{
			return wire_write(L"$stub_id",pMessage,m_proxy_id);
		}

		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL GetMarshaller_Safe(Omega::System::MetaInfo::IMarshaller_Safe** ppMarshaller)
		{
			*ppMarshaller = m_pManager;
			(*ppMarshaller)->AddRef_Safe();
			return 0;
		}

		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL IsAlive_Safe(Omega::bool_t* pRet);

	// IMarshal_Safe members
	public:
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL GetUnmarshalFactoryOID_Safe(Omega::guid_t* pRet, const Omega::guid_t*, Omega::Remoting::MarshalFlags_t)
		{
			*pRet = OID_ProxyMarshalFactory;
			return 0;
		}

		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL MarshalInterface_Safe(Omega::System::MetaInfo::interface_info<Omega::Remoting::IObjectManager>::safe_class* pObjectManager, Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t* piid, Omega::Remoting::MarshalFlags_t flags);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL ReleaseMarshalData_Safe(Omega::System::MetaInfo::interface_info<Omega::Remoting::IObjectManager>::safe_class* pObjectManager, Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t* piid, Omega::Remoting::MarshalFlags_t flags);

	private:
		Proxy(const Proxy&) : Omega::System::MetaInfo::IProxy_Safe(), Omega::System::MetaInfo::interface_info<Omega::Remoting::IMarshal>::safe_class() {}
		Proxy& operator = (const Proxy&) { return *this; }

		OOBase::AtomicInt<unsigned long> m_refcount;
		OOBase::AtomicInt<unsigned long> m_marshal_count;
		OOBase::RWMutex                  m_lock;
		Omega::uint32_t                  m_proxy_id;
		StdObjectManager*                m_pManager;

		std::map<const Omega::guid_t,Omega::System::MetaInfo::IObject_Safe*> m_iid_map;

		bool CallRemoteQI(const Omega::guid_t& iid);
		Omega::Remoting::IMessage* CallRemoteStubMarshal(Omega::Remoting::IObjectManager* pObjectManager, const Omega::guid_t& iid);
		void CallRemoteRelease();
	};
}

#endif // OOCORE_WIREPROXY_H_INCLUDED_
