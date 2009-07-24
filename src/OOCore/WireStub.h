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

#include <OOCore/Remoting.h>

namespace OOCore
{
	class StdObjectManager;

	class Stub : 
		public OTL::ObjectBase,
		public Omega::System::IStub,
		public Omega::System::IStubController
	{
	public:
		Stub();
				
		void init(Omega::IObject* pObj, Omega::uint32_t stub_id, StdObjectManager* pManager);

		void MarshalInterface(Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid);
		void ReleaseMarshalData(Omega::Remoting::IMessage* pMessage, const Omega::guid_t&);
		OTL::ObjectPtr<Omega::System::IStub> LookupStub(Omega::Remoting::IMessage* pMessage);

		BEGIN_INTERFACE_MAP(Stub)
			INTERFACE_ENTRY(Omega::System::IStub)
			INTERFACE_ENTRY(Omega::System::IStubController)
		END_INTERFACE_MAP()

	protected:
		void Internal_AddRef()
		{
			printf("WireStub %p AddRef > %u\n",this,m_refcount.m_debug_value+1);

			OTL::ObjectBase::Internal_AddRef();
		}

		void Internal_Release()
		{
			printf("WireStub %p Release < %u\n",this,m_refcount.m_debug_value-1);

			OTL::ObjectBase::Internal_Release();
		}

	private:
		Stub(const Stub&);
		Stub& operator = (const Stub&);

		OOBase::AtomicInt<Omega::uint32_t>     m_marshal_count;
		OOBase::SpinLock                       m_lock;
		Omega::uint32_t                        m_stub_id;
		OTL::ObjectPtr<Omega::IObject>         m_ptrObj;
		StdObjectManager*                      m_pManager;

		std::map<const Omega::guid_t,OTL::ObjectPtr<Omega::System::IStub> > m_iid_map;

		OTL::ObjectPtr<Omega::System::IStub> FindStub(const Omega::guid_t& iid);

	// IStub members
	public:
		void Invoke(Omega::Remoting::IMessage* pParamsIn, Omega::Remoting::IMessage* pParamsOut);
		Omega::bool_t SupportsInterface(const Omega::guid_t& /*iid*/) { return false; }

	// IStubController members
	public:
		void RemoteRelease(Omega::uint32_t release_count);
		Omega::bool_t RemoteQueryInterface(const Omega::guid_t& iid);
		void MarshalStub(Omega::Remoting::IMessage* pParamsIn, Omega::Remoting::IMessage* pParamsOut);
	};
}

#endif // OOCORE_WIRESTUB_H_INCLUDED_
