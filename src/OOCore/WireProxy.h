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

#include "../../include/Omega/Remoting.h"

namespace OOCore
{
	class StdObjectManager;

	// {69099DD8-A628-458a-861F-009E016DB81B}
	extern const Omega::guid_t OID_ProxyMarshalFactory;

	class ProxyMarshalFactory :
			public OTL::ObjectBase,
			public OTL::AutoObjectFactorySingleton<ProxyMarshalFactory,&OID_ProxyMarshalFactory,Omega::Activation::ProcessScope>,
			public Omega::Remoting::IMarshalFactory
	{
	public:
		BEGIN_INTERFACE_MAP(ProxyMarshalFactory)
			INTERFACE_ENTRY(Omega::Remoting::IMarshalFactory)
		END_INTERFACE_MAP()

	// IMarshalFactory members
	public:
		void UnmarshalInterface(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject);
	};

	class Proxy :
			public OTL::ObjectBase,
			public Omega::Remoting::IProxy,
			public Omega::Remoting::IMarshal
	{
	public:
		Proxy();
		
		void init(Omega::uint32_t proxy_id, StdObjectManager* pManager);
		void Final_Release();

		Omega::IObject* UnmarshalInterface(Omega::Remoting::IMessage* pMessage);

		BEGIN_INTERFACE_MAP(Proxy)
			INTERFACE_ENTRY(Omega::Remoting::IProxy)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
		END_INTERFACE_MAP()

	// IProxy members
	public:
		void WriteKey(Omega::Remoting::IMessage* pMessage)
		{
			pMessage->WriteValue(Omega::string_t::constant("$stub_id"),m_proxy_id);
		}

		void UnpackKey(Omega::Remoting::IMessage* pMessage)
		{
			pMessage->ReadValue(Omega::string_t::constant("$stub_id"));
		}

		Omega::Remoting::IMarshaller* GetMarshaller();
		Omega::bool_t IsAlive();
		Omega::bool_t RemoteQueryInterface(const Omega::guid_t& iid);
		Omega::IObject* QueryIObject();

	// IMarshal members
	public:
		Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			return OID_ProxyMarshalFactory;
		}

		void MarshalInterface(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		void ReleaseMarshalData(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);

	private:
		Proxy(const Proxy&);
		Proxy& operator = (const Proxy&);

		OOBase::SpinLock                      m_lock;
		Omega::uint32_t                       m_proxy_id;
		StdObjectManager*                     m_pManager;

		OOBase::HashTable<Omega::guid_t,bool,OOBase::HeapAllocator,GuidHash> m_iids;

		void WriteStubInfo(Omega::Remoting::IMessage* pMessage, Omega::uint32_t method_id);
		void ReadStubInfo(Omega::Remoting::IMessage* pMessage);
		Omega::Remoting::IMessage* CallRemoteStubMarshal(Omega::Remoting::IMarshaller* pMarshaller, const Omega::guid_t& iid);
		void CallRemoteRelease();
	};
}

#endif // OOCORE_WIREPROXY_H_INCLUDED_
