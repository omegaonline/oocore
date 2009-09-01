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

#include <OOCore/Remoting.h>

namespace OOCore
{
	class StdObjectManager;

	// {69099DD8-A628-458a-861F-009E016DB81B}
	extern "C" const Omega::guid_t OID_ProxyMarshalFactory;

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
		public OTL::ObjectBase,
		public Omega::System::IProxy,
		public Omega::System::MetaInfo::ISafeProxy,
		public Omega::Remoting::IMarshal
	{
	public:
		Proxy();
		virtual ~Proxy();

		void init(Omega::uint32_t proxy_id, StdObjectManager* pManager);

		void Disconnect();

		Omega::IObject* UnmarshalInterface(Omega::Remoting::IMessage* pMessage, const Omega::guid_t& wire_iid);

		BEGIN_INTERFACE_MAP(Proxy)
			INTERFACE_ENTRY(Omega::System::IProxy)
			INTERFACE_ENTRY(Omega::System::MetaInfo::ISafeProxy)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
		END_INTERFACE_MAP()

	// IProxy members
	public:
		void WriteKey(Omega::Remoting::IMessage* pMessage)
		{
			Omega::System::MetaInfo::wire_write(L"$stub_id",pMessage,m_proxy_id);
		}

		void UnpackKey(Omega::Remoting::IMessage* pMessage)
		{
			Omega::uint32_t k;
			Omega::System::MetaInfo::wire_read(L"$stub_id",pMessage,k);
		}

		Omega::System::IMarshaller* GetMarshaller();
		Omega::bool_t IsAlive();
		Omega::bool_t RemoteQueryInterface(const Omega::guid_t& iid);

		// IProxy_Safe members
		static const Omega::System::MetaInfo::SafeShim* OMEGA_CALL AddRef_Safe(const Omega::System::MetaInfo::SafeShim* shim);
		static const Omega::System::MetaInfo::SafeShim* OMEGA_CALL Release_Safe(const Omega::System::MetaInfo::SafeShim* shim);
		static const Omega::System::MetaInfo::SafeShim* OMEGA_CALL QueryInterface_Safe(const Omega::System::MetaInfo::SafeShim* shim, const Omega::System::MetaInfo::SafeShim** retval, const Omega::guid_base_t* iid);
		static const Omega::System::MetaInfo::SafeShim* OMEGA_CALL Pin_Safe(const Omega::System::MetaInfo::SafeShim* shim);
		static const Omega::System::MetaInfo::SafeShim* OMEGA_CALL Unpin_Safe(const Omega::System::MetaInfo::SafeShim* shim);
		static const Omega::System::MetaInfo::SafeShim* OMEGA_CALL GetBaseShim_Safe(const Omega::System::MetaInfo::SafeShim* shim, const Omega::System::MetaInfo::SafeShim** retval);
		static const Omega::System::MetaInfo::SafeShim* OMEGA_CALL WriteKey_Safe(const Omega::System::MetaInfo::SafeShim* shim, const Omega::System::MetaInfo::SafeShim* pMessage);
		static const Omega::System::MetaInfo::SafeShim* OMEGA_CALL UnpackKey_Safe(const Omega::System::MetaInfo::SafeShim* shim, const Omega::System::MetaInfo::SafeShim* pMessage);
		static const Omega::System::MetaInfo::SafeShim* OMEGA_CALL GetMarshaller_Safe(const Omega::System::MetaInfo::SafeShim* shim, const Omega::System::MetaInfo::SafeShim** retval);
		static const Omega::System::MetaInfo::SafeShim* OMEGA_CALL IsAlive_Safe(const Omega::System::MetaInfo::SafeShim* shim, int* retval);
		static const Omega::System::MetaInfo::SafeShim* OMEGA_CALL RemoteQueryInterface_Safe(const Omega::System::MetaInfo::SafeShim* shim, int* retval, const Omega::guid_base_t* iid);

	// ISafeProxy members
	public:
		void Internal_Release()
		{
			assert(m_refcount.m_debug_value > 0);

			if (m_refcount.Release() && m_pin_count == 0)
				delete this;
		}

		void Pin()
		{
			++m_pin_count;
		}

		void Unpin()
		{
			assert(m_pin_count.value() > 0);

			if (--m_pin_count == 0 && m_refcount.IsZero())
				delete this;
		}

		const Omega::System::MetaInfo::SafeShim* GetShim(const Omega::guid_t& iid);

		const Omega::System::MetaInfo::SafeShim* CreateWireStub(const Omega::System::MetaInfo::SafeShim*, const Omega::System::MetaInfo::SafeShim*, const Omega::guid_t&)
		{
			return 0;
		}

		Omega::System::IProxy* GetWireProxy()
		{
			Internal_AddRef();
			return static_cast<Omega::System::IProxy*>(this);
		}

	// IMarshal members
	public:
		Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			return OID_ProxyMarshalFactory;
		}

		void MarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		void ReleaseMarshalData(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);

		static const Omega::System::MetaInfo::SafeShim* OMEGA_CALL GetUnmarshalFactoryOID_Safe(const Omega::System::MetaInfo::SafeShim* shim, Omega::guid_base_t* retval, const Omega::guid_base_t* piid, Omega::Remoting::MarshalFlags_t flags);
		static const Omega::System::MetaInfo::SafeShim* OMEGA_CALL MarshalInterface_Safe(const Omega::System::MetaInfo::SafeShim* shim, const Omega::System::MetaInfo::SafeShim* pObjectManager, const Omega::System::MetaInfo::SafeShim* pMessage, const Omega::guid_base_t* iid, Omega::Remoting::MarshalFlags_t flags);
		static const Omega::System::MetaInfo::SafeShim* OMEGA_CALL ReleaseMarshalData_Safe(const Omega::System::MetaInfo::SafeShim* shim, const Omega::System::MetaInfo::SafeShim* pObjectManager, const Omega::System::MetaInfo::SafeShim* pMessage, const Omega::guid_base_t* iid, Omega::Remoting::MarshalFlags_t flags);

	private:
		Proxy(const Proxy&);
		Proxy& operator = (const Proxy&);

		OOBase::AtomicInt<Omega::uint32_t> m_marshal_count;
		OOBase::AtomicInt<Omega::uint32_t> m_pin_count;
		OOBase::SpinLock                   m_lock;
		Omega::uint32_t                    m_proxy_id;
		StdObjectManager*                  m_pManager;
		Omega::System::MetaInfo::SafeShim  m_proxy_shim;
		Omega::System::MetaInfo::SafeShim  m_marshal_shim;

		void WriteStubInfo(Omega::Remoting::IMessage* pMessage, Omega::uint32_t method_id);
		void ReadStubInfo(Omega::Remoting::IMessage* pMessage);
		Omega::Remoting::IMessage* CallRemoteStubMarshal(Omega::Remoting::IObjectManager* pObjectManager, const Omega::guid_t& iid);
		void CallRemoteRelease();
	};
}

#endif // OOCORE_WIREPROXY_H_INCLUDED_
