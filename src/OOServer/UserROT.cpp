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
	ObjectPtr<ObjectImpl<DuplicateRegistrationException> > pRE = ObjectImpl<DuplicateRegistrationException>::CreateInstance();
	pRE->m_strDesc = string_t::constant("Duplicate registration of oid {0} in running object table.") % oid;
	pRE->m_oid = oid;
	throw static_cast<IDuplicateRegistrationException*>(pRE.Detach());
}

User::RunningObjectTable::RunningObjectTable() : m_mapObjectsByCookie(1)
{
}

void User::RunningObjectTable::init(Remoting::IObjectManager* pOM)
{
	if (pOM)
	{
		// Create a proxy to the global interface
		IObject* pIPS = NULL;
		pOM->GetRemoteInstance(OOCore::OID_InterProcessService,Activation::Library | Activation::DontLaunch,OMEGA_GUIDOF(OOCore::IInterProcessService),pIPS);
		ObjectPtr<OOCore::IInterProcessService> ptrIPS = static_cast<OOCore::IInterProcessService*>(pIPS);

		// Get the running object table
		m_ptrROT = ptrIPS->GetRunningObjectTable();
	}
}

uint32_t User::RunningObjectTable::RegisterObject(const any_t& oid, IObject* pObject, Activation::RegisterFlags_t flags)
{
	uint32_t rot_cookie = 0;

	// Check for public registration...
	if (m_ptrROT && (flags & ~Activation::UserScope))
	{
		// Register in sandbox ROT
		rot_cookie = m_ptrROT->RegisterObject(oid,pObject,flags);
	}

	try
	{
		uint32_t src_id = 0;
		ObjectPtr<Remoting::ICallContext> ptrCC = Remoting::GetCallContext();
		if (ptrCC)
			src_id = ptrCC->SourceId();

		// Check if we have someone registered already
		OOBase::Stack<uint32_t,OOBase::LocalAllocator> revoke_list;
		string_t strOid = oid.cast<string_t>();

		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		for (size_t i=m_mapObjectsByOid.find_first(strOid); i<m_mapObjectsByOid.size() && *m_mapObjectsByOid.key_at(i)==strOid; ++i)
		{
			// Check its still alive...
			Info* pInfo = m_mapObjectsByCookie.find(*m_mapObjectsByOid.at(i));
			if (pInfo)
			{
				if (!Omega::Remoting::IsAlive(pInfo->m_ptrObject))
				{
					int err = revoke_list.push(*m_mapObjectsByOid.at(i));
					if (err != 0)
						OMEGA_THROW(err);
				}
				else
				{
					if (!(pInfo->m_flags & Activation::MultipleRegistration))
						DuplicateRegistrationException::Throw(oid);

					if (pInfo->m_flags == flags)
						DuplicateRegistrationException::Throw(oid);
				}
			}
		}

		// Create a new cookie
		Info info;
		info.m_oid = strOid;
		info.m_flags = flags;
		info.m_ptrObject = pObject;
		info.m_ptrObject.AddRef();
		info.m_rot_cookie = rot_cookie;
		info.m_source = src_id;

		uint32_t nCookie = 0;
		int err = m_mapObjectsByCookie.insert(info,nCookie,1,0xFFFFFFFF);
		if (err == 0)
		{
			err = m_mapObjectsByOid.insert(strOid,nCookie);
			if (err != 0)
				m_mapObjectsByCookie.remove(nCookie);
		}
		if (err != 0)
			OMEGA_THROW(err);

		guard.release();

		// Revoke the revoke_list
		for (uint32_t i = 0;revoke_list.pop(&i);)
			RevokeObject_i(i,0);

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
	ObjectPtr<IObject> ptrObject;

	// Strip off the option flags
	Activation::RegisterFlags_t search_flags = flags & 0xF;

	OOBase::Stack<uint32_t,OOBase::LocalAllocator> revoke_list;
	string_t strOid = oid.cast<string_t>();

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	for (size_t i=m_mapObjectsByOid.find_first(strOid); i<m_mapObjectsByOid.size() && *m_mapObjectsByOid.key_at(i)==strOid;++i)
	{
		Info* pInfo = m_mapObjectsByCookie.find(*m_mapObjectsByOid.at(i));
		if (pInfo && (pInfo->m_flags & search_flags))
		{
			// Check its still alive...
			if (!Omega::Remoting::IsAlive(pInfo->m_ptrObject))
			{
				revoke_list.push(*m_mapObjectsByOid.at(i));
			}
			else
			{
				ptrObject = pInfo->m_ptrObject->QueryInterface(iid);
				if (!ptrObject)
					throw OOCore_INotFoundException_MissingIID(iid);

				// Remove the entry if Activation::SingleUse
				if (pInfo->m_flags & Activation::SingleUse)
				{
					int err = revoke_list.push(*m_mapObjectsByOid.at(i));
					if (err != 0)
						OMEGA_THROW(err);
				}
				break;
			}
		}
	}

	guard.release();

	// Revoke the revoke_list
	for (uint32_t i = 0;revoke_list.pop(&i);)
		RevokeObject_i(i,0);

	if (ptrObject)
	{
		pObject = ptrObject.Detach();
	}
	else if (m_ptrROT && (flags & ~Activation::UserScope))
	{
		// Route to global rot
		m_ptrROT->GetObject(oid,flags,iid,pObject);
	}
}

void User::RunningObjectTable::RevokeObject_i(uint32_t cookie, uint32_t src_id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	Info* pInfo = m_mapObjectsByCookie.find(cookie);
	if (pInfo && (!src_id || pInfo->m_source == src_id))
	{
		uint32_t rot_cookie = pInfo->m_rot_cookie;

		for (size_t i=m_mapObjectsByOid.find_first(pInfo->m_oid); i<m_mapObjectsByOid.size() && *m_mapObjectsByOid.key_at(i)==pInfo->m_oid; ++i)
		{
			if (*m_mapObjectsByOid.at(i) == cookie)
				m_mapObjectsByOid.remove_at(i);
		}

		m_mapObjectsByCookie.remove(cookie);

		guard.release();

		if (rot_cookie && m_ptrROT)
			m_ptrROT->RevokeObject(rot_cookie);
	}
}

void User::RunningObjectTable::RevokeObject(uint32_t cookie)
{
	uint32_t src_id = 0;
	ObjectPtr<Remoting::ICallContext> ptrCC = Remoting::GetCallContext();
	if (ptrCC)
		src_id = ptrCC->SourceId();

	RevokeObject_i(cookie,src_id);
}
