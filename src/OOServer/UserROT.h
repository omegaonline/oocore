///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
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

#ifndef OOSERVER_USER_SERVICE_TABLE_H_INCLUDED_
#define OOSERVER_USER_SERVICE_TABLE_H_INCLUDED_

namespace User
{
	class RunningObjectTable :
		public OTL::ObjectBase,
		public Omega::Activation::IRunningObjectTable
	{
	public:
		RunningObjectTable() {}

		void Init(OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOM);

		void Register(const Omega::guid_t& oid, Omega::Activation::IRunningObjectTable::Flags_t flags, Omega::IObject* pObject);
		void Revoke(const Omega::guid_t& oid);
		Omega::IObject* GetObject(const Omega::guid_t& oid);

		BEGIN_INTERFACE_MAP(RunningObjectTable)
			INTERFACE_ENTRY(Omega::Activation::IRunningObjectTable)
		END_INTERFACE_MAP()

	private:
		RunningObjectTable(const RunningObjectTable&) : OTL::ObjectBase(), Omega::Activation::IRunningObjectTable() {}
		RunningObjectTable& operator = (const RunningObjectTable&) { return *this; }

		OTL::ObjectPtr<Omega::Activation::IRunningObjectTable>   m_ptrROT;
		ACE_RW_Thread_Mutex                                      m_lock;
		std::map<Omega::guid_t,OTL::ObjectPtr<Omega::IObject> >  m_mapServices;
	};
}

#endif // OOSERVER_USER_SERVICE_TABLE_H_INCLUDED_
