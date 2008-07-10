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

#include "./OOServer_User.h"
#include "./UserROT.h"

#include "../OOCore/Server.h"

using namespace Omega;
using namespace OTL;

User::RunningObjectTable::RunningObjectTable() : m_nNextCookie(1)
{
}

void User::RunningObjectTable::Init(ObjectPtr<Remoting::IObjectManager> ptrOM)
{
	if (ptrOM)
	{
		// Create a proxy to the global interface
		IObject* pIPS = 0;
		ptrOM->GetRemoteInstance(Remoting::OID_InterProcessService,Activation::InProcess | Activation::DontLaunch,OMEGA_UUIDOF(Remoting::IInterProcessService),pIPS);
		ObjectPtr<Remoting::IInterProcessService> ptrIPS;
		ptrIPS.Attach(static_cast<Remoting::IInterProcessService*>(pIPS));

		// Get the running object table
		m_ptrROT.Attach(ptrIPS->GetRunningObjectTable());
	}
}

uint32_t User::RunningObjectTable::Register(const guid_t& oid, IObject* pObject)
{
	// Never allow registration in the global ROT!
	// If we are a client of the sandbox we will register there (which is correct)
	// But any other condition will result in a very easy way to achieve priviledge escalation
	// You have been warned!

	try
	{
		void* TICKET_99;

		uint32_t src_id = 0;
		ObjectPtr<Remoting::ICallContext> ptrCC;
		ptrCC.Attach(Remoting::GetCallContext());
		if (ptrCC != 0)
			src_id = ptrCC->SourceId();

		OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		// Create a new cookie
		Info info;
		info.m_oid = oid;
		info.m_ptrObject = pObject;
		info.m_source = src_id;
		uint32_t nCookie = m_nNextCookie++;
		while (nCookie==0 && m_mapObjectsByCookie.find(nCookie) != m_mapObjectsByCookie.end())
		{
			nCookie = m_nNextCookie++;
		}

		std::pair<std::map<uint32_t,Info>::iterator,bool> p = m_mapObjectsByCookie.insert(std::map<uint32_t,Info>::value_type(nCookie,info));
		if (!p.second)
			OMEGA_THROW(EALREADY);

		m_mapObjectsByOid.insert(std::multimap<guid_t,std::map<uint32_t,Info>::iterator>::value_type(oid,p.first));

		return nCookie;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void User::RunningObjectTable::Revoke(uint32_t cookie)
{
	try
	{
		uint32_t src_id = 0;
		ObjectPtr<Remoting::ICallContext> ptrCC;
		ptrCC.Attach(Remoting::GetCallContext());
		if (ptrCC != 0)
			src_id = ptrCC->SourceId();

		OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,Info>::iterator i = m_mapObjectsByCookie.find(cookie);
		if (i != m_mapObjectsByCookie.end() && i->second.m_source == src_id)
		{
			for (std::multimap<guid_t,std::map<uint32_t,Info>::iterator>::iterator j=m_mapObjectsByOid.find(i->second.m_oid);j!=m_mapObjectsByOid.end() && j->first==i->second.m_oid;++j)
			{
				if (j->second->first == cookie)
				{
					m_mapObjectsByOid.erase(j);
					break;
				}
			}
			m_mapObjectsByCookie.erase(i);
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

IObject* User::RunningObjectTable::GetObject(const guid_t& oid)
{
	try
	{
		ObjectPtr<IObject> ptrRet;
		std::list<uint32_t> listDeadEntries;
		{
			OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			for (std::multimap<guid_t,std::map<uint32_t,Info>::iterator>::iterator i=m_mapObjectsByOid.find(oid);i!=m_mapObjectsByOid.end() && i->first==oid;++i)
			{
				// QI for IWireProxy and check its still there!
				bool bOk = true;
				ObjectPtr<System::MetaInfo::IWireProxy> ptrProxy = (IObject*)i->second->second.m_ptrObject;
				if (ptrProxy != 0)
				{
					if (!ptrProxy->IsAlive())
					{
						listDeadEntries.push_back(i->second->first);
						bOk = false;
					}
				}

				if (bOk)
					ptrRet = i->second->second.m_ptrObject;
			}
		}

		if (!listDeadEntries.empty())
		{
			// We found at least one dead proxy
			OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			for (std::list<uint32_t>::iterator i=listDeadEntries.begin();i!=listDeadEntries.end();++i)
			{
				std::map<uint32_t,Info>::iterator j = m_mapObjectsByCookie.find(*i);
				for (std::multimap<guid_t,std::map<uint32_t,Info>::iterator>::iterator k=m_mapObjectsByOid.find(j->second.m_oid);k!=m_mapObjectsByOid.end() && k->first==j->second.m_oid;++k)
				{
					if (k->second->first == *i)
					{
						m_mapObjectsByOid.erase(k);
						break;
					}
				}
				m_mapObjectsByCookie.erase(j);
			}
		}

		if (ptrRet != 0)
			return ptrRet.AddRef();
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	if (m_ptrROT != 0)
	{
		// Route to global rot
		return m_ptrROT->GetObject(oid);
	}

	return 0;
}
