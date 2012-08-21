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
	void RegisterObjects();
	void UnregisterObjects(bool bPrivate);

	OTL::ObjectPtr<OOCore::IInterProcessService> GetInterProcessService(bool bThrow);

	Omega::IObject* GetInstance(const Omega::any_t& oid, Omega::Activation::Flags_t flags, const Omega::guid_t& iid);

	// The instance wide LocalROT instance
	class LocalROT : 
		public Omega::Activation::IRunningObjectTable,
		public Omega::Activation::IRunningObjectTableNotify,
		public Omega::Notify::INotifier
	{
	public:
		LocalROT();

		OTL::ObjectPtr<OOCore::IInterProcessService> GetIPS(bool bThrow);
		OTL::ObjectPtr<Omega::Registry::IKey> GetRootKey();
		void RegisterIPS(OOCore::IInterProcessService* pIPS);
		bool IsHosted();
		void RevokeIPS();

		static LocalROT* instance()
		{
			return OOBase::Singleton<LocalROT,OOCore::DLL>::instance_ptr();
		}

	private:
		LocalROT(const LocalROT&);
		LocalROT& operator = (const LocalROT&);

		OOBase::RWMutex m_lock;
		Omega::uint32_t m_notify_cookie;
		bool m_hosted_by_ooserver;

		OTL::ObjectPtr<OOCore::IInterProcessService>           m_ptrIPS;
		OTL::ObjectPtr<Omega::Registry::IKey>                  m_ptrReg;
		OTL::ObjectPtr<Omega::Activation::IRunningObjectTable> m_ptrROT;

		struct Info
		{
			Omega::string_t                    m_oid;
			OTL::ObjectPtr<Omega::IObject>     m_ptrObject;
			Omega::Activation::RegisterFlags_t m_flags;
			Omega::uint32_t                    m_rot_cookie;
		};
		OOBase::HandleTable<Omega::uint32_t,Info>      m_mapServicesByCookie;
		OOBase::Table<Omega::string_t,Omega::uint32_t> m_mapServicesByOid;

		OOBase::HandleTable<Omega::uint32_t,OTL::ObjectPtr<Omega::Activation::IRunningObjectTableNotify> > m_mapNotify;

		OTL::ObjectPtr<Omega::Activation::IRunningObjectTable> GetROT(bool bThrow);

	// IObject members
	public:
		void AddRef()
		{
			OTL::GetModule()->IncLockCount();
		}

		void Release()
		{
			OTL::GetModule()->DecLockCount();
		}

		Omega::IObject* QueryInterface(const Omega::guid_t& iid);

	// IRunningObjectTable members
	public:
		Omega::uint32_t RegisterObject(const Omega::any_t& oid, Omega::IObject* pObject, Omega::Activation::RegisterFlags_t flags);
		void GetObject(const Omega::any_t& oid, const Omega::guid_t& iid, Omega::IObject*& pObject);
		void RevokeObject(Omega::uint32_t cookie);

	// IRunningObjectTableNotify members
	public:
		void OnRegisterObject(const Omega::any_t& oid, Omega::Activation::RegisterFlags_t flags);
		void OnRevokeObject(const Omega::any_t& oid, Omega::Activation::RegisterFlags_t flags);

	// INotifier members
	public:
		Omega::uint32_t RegisterNotify(const Omega::guid_t& iid, Omega::IObject* pObject);
		void UnregisterNotify(const Omega::guid_t& iid, Omega::uint32_t cookie);
		iid_list_t ListNotifyInterfaces();
	};

	class RunningObjectTableFactory : 
		public OTL::ObjectFactoryBase<&Omega::Activation::OID_RunningObjectTable,Omega::Activation::ProcessScope>
	{
	// IObjectFactory members
	public:
		void CreateInstance(const Omega::guid_t& iid, Omega::IObject*& pObject);
	};
	
	class RegistryFactory : 
		public OTL::ObjectFactoryBase<&Omega::Registry::OID_Registry,Omega::Activation::ProcessScope>
	{
	// IObjectFactory members
	public:
		void CreateInstance(const Omega::guid_t& iid, Omega::IObject*& pObject);
	};
}

#endif // OOCORE_ACTIVATION_H_INCLUDED_
