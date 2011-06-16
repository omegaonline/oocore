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

#ifndef OOCORE_ACTIVATION_H_INCLUDED_
#define OOCORE_ACTIVATION_H_INCLUDED_

#include "Server.h"

namespace OOCore
{
	OTL::ObjectPtr<IInterProcessService> GetInterProcessService();

	Omega::IObject* GetInstance(const Omega::any_t& oid, Omega::Activation::Flags_t flags, const Omega::guid_t& iid);

	// The instance wide ServiceManager instance
	class ServiceManager : 
		public OTL::ObjectBase,
		public Omega::Activation::IRunningObjectTable
	{
	public:
		OTL::ObjectPtr<OOCore::IInterProcessService> GetIPS();
		
	protected:
		ServiceManager();
		virtual ~ServiceManager();
	
		BEGIN_INTERFACE_MAP(ServiceManager)
			INTERFACE_ENTRY(Omega::Activation::IRunningObjectTable)
		END_INTERFACE_MAP()

	private:
		ServiceManager(const ServiceManager&);
		ServiceManager& operator = (const ServiceManager&);

		OOBase::RWMutex m_lock;
		
		struct Info
		{
			Omega::string_t                    m_oid;
			OTL::ObjectPtr<Omega::IObject>     m_ptrObject;
			Omega::Activation::RegisterFlags_t m_flags;
			Omega::uint32_t                    m_rot_cookie;
		};
		OOBase::HandleTable<Omega::uint32_t,Info>      m_mapServicesByCookie;
		OOBase::Table<Omega::string_t,Omega::uint32_t> m_mapServicesByOid;

	// IRunningObjectTable members
	public:
		Omega::uint32_t RegisterObject(const Omega::any_t& oid, Omega::IObject* pObject, Omega::Activation::RegisterFlags_t flags);
		void GetObject(const Omega::any_t& oid, Omega::Activation::RegisterFlags_t flags, const Omega::guid_t& iid, Omega::IObject*& pObject);
		void RevokeObject(Omega::uint32_t cookie);
	};

	// {0CA3037F-08C0-442a-B4EC-84A9156839CD}
	extern "C" const Omega::guid_t OID_OidNotFoundExceptionMarshalFactory;

	class OidNotFoundException :
		public OTL::ExceptionAutoMarshalImpl<Omega::Activation::IOidNotFoundException, &OID_OidNotFoundExceptionMarshalFactory>
	{
		typedef OTL::ExceptionAutoMarshalImpl<Omega::Activation::IOidNotFoundException, &OID_OidNotFoundExceptionMarshalFactory> baseClass;
	public:
		static void Throw(const Omega::any_t& oid, Omega::IException* pE = 0);

		BEGIN_INTERFACE_MAP(OidNotFoundException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()

		Omega::any_t m_oid;

		virtual void UnmarshalInterface(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::UnmarshalInterface(pMarshaller,pMessage,flags);
			m_oid = pMessage->ReadValue(L"m_oid");
		}

		virtual void MarshalInterface(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::MarshalInterface(pMarshaller,pMessage,iid,flags);
			pMessage->WriteValue(L"m_oid",m_oid);
		}

		virtual void ReleaseMarshalData(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::ReleaseMarshalData(pMarshaller,pMessage,iid,flags);
			pMessage->ReadValue(L"m_oid");
		}

	// Activation::IOidNotFoundException members
	public:
		Omega::any_t GetMissingOid()
		{
			return m_oid;
		}
	};

	class OidNotFoundExceptionMarshalFactoryImpl :
			public OTL::AutoObjectFactorySingleton<OidNotFoundExceptionMarshalFactoryImpl,&OID_OidNotFoundExceptionMarshalFactory,Omega::Activation::InProcess>,
			public OTL::ExceptionMarshalFactoryImpl<OidNotFoundException>
	{
	};
		
	class RunningObjectTableFactory : 
		public OTL::ObjectFactoryBase<&Omega::Activation::OID_RunningObjectTableFactory,Omega::Activation::InProcess>
	{
	// IObjectFactory members
	public:
		void CreateInstance(Omega::IObject* pOuter, const Omega::guid_t& iid, Omega::IObject*& pObject);
	};
}

#endif // OOCORE_ACTIVATION_H_INCLUDED_
