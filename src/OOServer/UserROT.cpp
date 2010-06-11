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

#include "OOServer_User.h"
#include "UserROT.h"

#include "../OOCore/Server.h"

using namespace Omega;
using namespace OTL;

namespace User
{
	class DuplicateRegistrationException :
			public ExceptionImpl<Activation::IDuplicateRegistrationException>
	{
	public:
		static void Throw(const any_t& oid);

		BEGIN_INTERFACE_MAP(DuplicateRegistrationException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<Activation::IDuplicateRegistrationException>)
		END_INTERFACE_MAP()

	private:
		any_t m_oid;

	// Activation::IDuplicateRegistrationException members
	public:
		any_t GetOid()
		{
			return m_oid;
		}
	};
}

void User::DuplicateRegistrationException::Throw(const any_t& oid)
{
	ObjectImpl<DuplicateRegistrationException>* pRE = ObjectImpl<DuplicateRegistrationException>::CreateInstance();
	pRE->m_strDesc = L"Duplicate registration of oid {0} in running object table.";
	pRE->m_strDesc %= oid;
	pRE->m_oid = oid;
	throw static_cast<IDuplicateRegistrationException*>(pRE);
}

User::RunningObjectTable::RunningObjectTable() : m_nNextCookie(1)
{
}

void User::RunningObjectTable::Init(ObjectPtr<Remoting::IObjectManager> ptrOM)
{
	if (ptrOM)
	{
		// Create a proxy to the global interface
		IObject* pIPS = 0;
		ptrOM->GetRemoteInstance(OOCore::OID_InterProcessService,Activation::InProcess | Activation::DontLaunch,OMEGA_GUIDOF(OOCore::IInterProcessService),pIPS);
		ObjectPtr<OOCore::IInterProcessService> ptrIPS;
		ptrIPS.Attach(static_cast<OOCore::IInterProcessService*>(pIPS));

		// Get the running object table
		m_ptrROT.Attach(ptrIPS->GetRunningObjectTable());
	}
}

uint32_t User::RunningObjectTable::RegisterObject(const any_t& oid, IObject* pObject, Activation::RegisterFlags_t flags)
{
	uint32_t rot_cookie = 0;

	// Check for public registration...
	if (m_ptrROT && (flags & (Activation::MachineLocal | Activation::Anywhere)))
	{
		// Register in sandbox ROT
		rot_cookie = m_ptrROT->RegisterObject(oid,pObject,flags);
	}

	try
	{
		uint32_t src_id = 0;
		ObjectPtr<Remoting::ICallContext> ptrCC;
		ptrCC.Attach(Remoting::GetCallContext());
		if (ptrCC != 0)
			src_id = ptrCC->SourceId();

		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		// Check if we have someone registered already
		string_t strOid = oid.cast<string_t>();
		for (std::multimap<string_t,std::map<uint32_t,Info>::iterator>::iterator i=m_mapObjectsByOid.find(strOid); i!=m_mapObjectsByOid.end() && i->first==strOid; ++i)
		{
			if (!(i->second->second.m_flags & Activation::MultipleRegistration))
				DuplicateRegistrationException::Throw(oid);

			if (i->second->second.m_flags == flags)
				DuplicateRegistrationException::Throw(oid);
		}

		// Create a new cookie
		Info info;
		info.m_oid = strOid;
		info.m_flags = flags;
		info.m_ptrObject = pObject;
		info.m_rot_cookie = rot_cookie;
		info.m_source = src_id;
		uint32_t nCookie = m_nNextCookie++;
		while (nCookie==0 && m_mapObjectsByCookie.find(nCookie) != m_mapObjectsByCookie.end())
		{
			nCookie = m_nNextCookie++;
		}

		std::pair<std::map<uint32_t,Info>::iterator,bool> p = m_mapObjectsByCookie.insert(std::map<uint32_t,Info>::value_type(nCookie,info));
		assert(p.second);

		m_mapObjectsByOid.insert(std::multimap<string_t,std::map<uint32_t,Info>::iterator>::value_type(strOid,p.first));

		return nCookie;
	}
	catch (...)
	{
		if (rot_cookie && m_ptrROT)
			m_ptrROT->RevokeObject(rot_cookie);

		throw;
	}
}

void User::RunningObjectTable::GetObject(const any_t& oid, Activation::RegisterFlags_t flags, const guid_t& iid, IObject*& pObject)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	bool bDead = false;
	string_t strOid = oid.cast<string_t>();
	for (std::multimap<string_t,std::map<uint32_t,Info>::iterator>::iterator i=m_mapObjectsByOid.find(strOid); i!=m_mapObjectsByOid.end() && i->first==strOid;++i)
	{
		if ((i->second->second.m_flags & flags))
		{
			// Check its still alive...
			if (!Omega::Remoting::IsAlive(i->second->second.m_ptrObject))
			{
				bDead = true;
				break;
			}
			else
			{
				pObject = i->second->second.m_ptrObject->QueryInterface(iid);
				if (!pObject)
					throw INoInterfaceException::Create(iid);
				return;
			}
		}
	}

	guard.release();

	if (bDead)
	{
		OOBase::Guard<OOBase::RWMutex> guard2(m_lock);

		std::multimap<string_t,std::map<uint32_t,Info>::iterator>::iterator i=m_mapObjectsByOid.find(strOid);
		
		// Remove it, its dead
		m_mapObjectsByCookie.erase(i->second);

		while (i!=m_mapObjectsByOid.end() && i->first==strOid)
			m_mapObjectsByOid.erase(i++);
	}

	if (m_ptrROT && (flags & (Activation::MachineLocal | Activation::Anywhere)))
	{
		// Route to global rot
		m_ptrROT->GetObject(oid,flags,iid,pObject);
	}
}

void User::RunningObjectTable::RevokeObject(uint32_t cookie)
{
	uint32_t src_id = 0;
	ObjectPtr<Remoting::ICallContext> ptrCC;
	ptrCC.Attach(Remoting::GetCallContext());
	if (ptrCC != 0)
		src_id = ptrCC->SourceId();

	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	std::map<uint32_t,Info>::iterator i = m_mapObjectsByCookie.find(cookie);
	if (i != m_mapObjectsByCookie.end() && i->second.m_source == src_id)
	{
		uint32_t rot_cookie = i->second.m_rot_cookie;

		for (std::multimap<string_t,std::map<uint32_t,Info>::iterator>::iterator j=m_mapObjectsByOid.find(i->second.m_oid); j!=m_mapObjectsByOid.end() && j->first==i->second.m_oid; ++j)
		{
			if (j->second->first == cookie)
			{
				m_mapObjectsByOid.erase(j);
				break;
			}
		}

		m_mapObjectsByCookie.erase(i);

		guard.release();

		if (rot_cookie && m_ptrROT)
			m_ptrROT->RevokeObject(rot_cookie);
	}
}
