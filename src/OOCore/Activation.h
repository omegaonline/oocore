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
	void UnregisterObjects();

	IInterProcessService* GetInterProcessService();

	Omega::IObject* GetInstance(const Omega::any_t& oid, Omega::Activation::Flags_t flags, const Omega::guid_t& iid);

	// The instance wide LocalROT instance
	class LocalROT : 
		public OTL::ObjectBase,
		public Omega::Activation::IRunningObjectTable
	{
	public:
		OOCore::IInterProcessService* GetIPS();

	protected:
		LocalROT();
		virtual ~LocalROT();
	
		BEGIN_INTERFACE_MAP(LocalROT)
			INTERFACE_ENTRY(Omega::Activation::IRunningObjectTable)
		END_INTERFACE_MAP()

	private:
		LocalROT(const LocalROT&);
		LocalROT& operator = (const LocalROT&);

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
		void GetObject(const Omega::any_t& oid, const Omega::guid_t& iid, Omega::IObject*& pObject, Omega::bool_t remote);
		void RevokeObject(Omega::uint32_t cookie);
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
