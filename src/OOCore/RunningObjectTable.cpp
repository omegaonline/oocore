///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
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

#include "OOCore_precomp.h"

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	class RunningObjectTable :
		public ObjectBase,
		public Activation::IRunningObjectTable
	{
	public:
		RunningObjectTable() {}

		void Register(const guid_t& oid, Activation::IRunningObjectTable::Flags_t flags, IObject* pObject);
		void Revoke(const guid_t& oid);
		IObject* GetObject(const guid_t& oid);

		BEGIN_INTERFACE_MAP(RunningObjectTable)
			INTERFACE_ENTRY(Activation::IRunningObjectTable)
		END_INTERFACE_MAP()

	private:
		RunningObjectTable(const RunningObjectTable&) : ObjectBase(), Activation::IRunningObjectTable() {}
		RunningObjectTable& operator = (const RunningObjectTable&) { return *this; }

		ACE_RW_Thread_Mutex                   m_lock;
		std::map<guid_t,ObjectPtr<IObject> >  m_mapServices;
	};

	// The instance wide RunningObjectTable instance
	struct ROT
	{
		ObjectPtr<Activation::IRunningObjectTable>  m_ptrSystemROT;
		ACE_Thread_Mutex                            m_lock;

		static ROT& instance()
		{
			static ROT i;
			return i;
		}
	};
	
	void SetRunningObjectTable(Activation::IRunningObjectTable* pNewTable);
}

// RunningObjectTable
void OOCore::SetRunningObjectTable(Activation::IRunningObjectTable* pNewTable)
{
	OOCORE_GUARD(ACE_Thread_Mutex,guard,ROT::instance().m_lock);

	ROT::instance().m_ptrSystemROT = pNewTable;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Activation::IRunningObjectTable*,Activation_GetRunningObjectTable,0,())
{
	// If we have no set RunningObjectTable, use a default
	if (!OOCore::ROT::instance().m_ptrSystemROT)
	{
		// Do a double lock here...
		OOCORE_GUARD(ACE_Thread_Mutex,guard,OOCore::ROT::instance().m_lock);

		if (!OOCore::ROT::instance().m_ptrSystemROT)
		{
			OOCore::ROT::instance().m_ptrSystemROT.Attach(ObjectImpl<OOCore::RunningObjectTable>::CreateInstance());
		}
	}

	return OOCore::ROT::instance().m_ptrSystemROT.AddRefReturn();
}

void OOCore::RunningObjectTable::Register(const guid_t& oid, Activation::IRunningObjectTable::Flags_t, IObject* pObject)
{
	OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	try
	{
		if (m_mapServices.find(oid) != m_mapServices.end())
			OOCORE_THROW_ERRNO(EALREADY);

		m_mapServices.insert(std::map<guid_t,ObjectPtr<IObject> >::value_type(oid,pObject));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

void OOCore::RunningObjectTable::Revoke(const guid_t& oid)
{
	OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	try
	{
		std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapServices.find(oid);
		if (i == m_mapServices.end())
			OOCORE_THROW_ERRNO(EINVAL);

		m_mapServices.erase(i);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

IObject* OOCore::RunningObjectTable::GetObject(const guid_t& oid)
{
	OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	try
	{
		std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapServices.find(oid);
		if (i == m_mapServices.end())
			OOCORE_THROW_ERRNO(EINVAL);

		return i->second.AddRefReturn();
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}
