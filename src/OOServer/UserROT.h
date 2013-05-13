///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOSERVER_RUNNING_OBJECT_TABLE_H_INCLUDED_
#define OOSERVER_RUNNING_OBJECT_TABLE_H_INCLUDED_

#include "../../include/Omega/Remoting.h"
#include "../../include/Omega/Notify.h"

namespace User
{
	class RunningObjectTable :
			public OTL::ObjectBase,
			public OTL::IProvideObjectInfoImpl<RunningObjectTable>,
			public Omega::Activation::IRunningObjectTable,
			public Omega::Activation::IRunningObjectTableNotify,
			public Omega::Notify::INotifier,
			public OOBase::NonCopyable
	{
	public:
		RunningObjectTable();

		void init(OTL::ObjectPtr<OOCore::IInterProcessService> ptrIPS);

		BEGIN_INTERFACE_MAP(RunningObjectTable)
			INTERFACE_ENTRY(Omega::Activation::IRunningObjectTable)
			INTERFACE_ENTRY(Omega::Activation::IRunningObjectTableNotify)
			INTERFACE_ENTRY(Omega::Notify::INotifier)
			INTERFACE_ENTRY(Omega::TypeInfo::IProvideObjectInfo)
		END_INTERFACE_MAP()

	private:
		OOBase::RWMutex                                        m_lock;
		OTL::ObjectPtr<Omega::Activation::IRunningObjectTable> m_ptrROT;
		Omega::uint32_t                                        m_notify_cookie;
		
		struct Info
		{
			Omega::string_t                      m_oid;
			OTL::ObjectPtr<Omega::IObject>       m_ptrObject;
			Omega::uint32_t                      m_source;
			Omega::Activation::RegisterFlags_t   m_flags;
			Omega::uint32_t                      m_rot_cookie;
		};
		OOBase::HandleTable<Omega::uint32_t,Info>      m_mapObjectsByCookie;
		OOBase::Table<Omega::string_t,Omega::uint32_t> m_mapObjectsByOid;

		OOBase::HandleTable<Omega::uint32_t,OTL::ObjectPtr<Omega::Activation::IRunningObjectTableNotify> > m_mapNotify;

		void RevokeObject_i(Omega::uint32_t cookie, Omega::uint32_t src_id);

	// IRunningObjectTable
	public:
		Omega::uint32_t RegisterObject(const Omega::any_t& oid, Omega::IObject* pObject, Omega::Activation::RegisterFlags_t reg_flags);
		void RevokeObject(Omega::uint32_t cookie);
		void GetObject(const Omega::any_t& oid, const Omega::guid_t& iid, Omega::IObject*& pObject);

	// IRunningObjectTableNotify members
	public:
		void OnRegisterObject(const Omega::any_t& oid, Omega::Activation::RegisterFlags_t flags);
		void OnRevokeObject(const Omega::any_t& oid, Omega::Activation::RegisterFlags_t flags);

	// INotifier members
	public:
		Omega::uint32_t RegisterNotify(const Omega::guid_t& iid, Omega::IObject* pObject);
		void UnregisterNotify(Omega::uint32_t cookie);
		Omega::Notify::INotifier::iid_list_t ListNotifyInterfaces();
	};
}

#endif // OOSERVER_RUNNING_OBJECT_TABLE_H_INCLUDED_
