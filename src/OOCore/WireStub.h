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

#ifndef OOCORE_WIRESTUB_H_INCLUDED_
#define OOCORE_WIRESTUB_H_INCLUDED_

#include "../../include/Omega/Remoting.h"

namespace OOCore
{
	class StdObjectManager;

	class Stub :
			public OTL::ObjectBase,
			public Omega::Remoting::IStubController
	{
	public:
		Stub();

		void init(Omega::IObject* pObj, Omega::uint32_t stub_id, StdObjectManager* pManager);

		void MarshalInterface(Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid);
		void ReleaseMarshalData(Omega::Remoting::IMessage* pMessage, const Omega::guid_t&);
		void Invoke(Omega::Remoting::IMessage* pParamsIn, Omega::Remoting::IMessage* pParamsOut);

		BEGIN_INTERFACE_MAP(Stub)
			INTERFACE_ENTRY(Omega::Remoting::IStubController)
		END_INTERFACE_MAP()

	private:
		Stub(const Stub&);
		Stub& operator = (const Stub&);

		OOBase::Atomic<Omega::uint32_t>     m_marshal_count;
		OOBase::SpinLock                    m_lock;
		Omega::uint32_t                     m_stub_id;
		OTL::ObjectPtr<Omega::IObject>      m_ptrObj;
		StdObjectManager*                   m_pManager;

		OOBase::HashTable<Omega::guid_t,OTL::ObjectPtr<Omega::Remoting::IStub>,OOBase::HeapAllocator,GuidHash> m_iid_map;

		Omega::Remoting::IStub* FindStub(const Omega::guid_t& iid);
		Omega::Remoting::IStub* CreateStub(const Omega::guid_t& iid);

	// IStubController members
	public:
		void RemoteRelease();
		Omega::bool_t RemoteQueryInterface(const Omega::guid_t& iid);
		void MarshalStub(Omega::Remoting::IMessage* pParamsIn, Omega::Remoting::IMessage* pParamsOut);
	};
}

#endif // OOCORE_WIRESTUB_H_INCLUDED_
