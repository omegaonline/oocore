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

#include <OOCore/Remoting.h>

namespace User
{
	class RunningObjectTable :
		public OTL::ObjectBase,
		public Omega::Activation::IRunningObjectTable
	{
	public:
		RunningObjectTable();

		void Init(OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOM);

		Omega::uint32_t Register(const Omega::guid_t& oid, Omega::IObject* pObject);
		void Revoke(Omega::uint32_t cookie);
		Omega::IObject* GetObject(const Omega::guid_t& oid);

		BEGIN_INTERFACE_MAP(RunningObjectTable)
			INTERFACE_ENTRY(Omega::Activation::IRunningObjectTable)
		END_INTERFACE_MAP()

	private:
		RunningObjectTable(const RunningObjectTable&);
		RunningObjectTable& operator = (const RunningObjectTable&);

		OTL::ObjectPtr<Omega::Activation::IRunningObjectTable> m_ptrROT;
		OOBase::RWMutex                                        m_lock;
		Omega::uint32_t                                        m_nNextCookie;

		struct Info
		{
			Omega::guid_t                      m_oid;
			OTL::ObjectPtr<Omega::IObject>     m_ptrObject;
			Omega::uint32_t                    m_source;
		};
		std::map<Omega::uint32_t,Info>                                        m_mapObjectsByCookie;
		std::multimap<Omega::guid_t,std::map<Omega::uint32_t,Info>::iterator> m_mapObjectsByOid;
	};
}

#endif // OOSERVER_RUNNING_OBJECT_TABLE_H_INCLUDED_
