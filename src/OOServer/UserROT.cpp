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

using namespace Omega;
using namespace OTL;

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
	Activation::RegisterFlags_t scope = (flags & 0xF);
	if (m_ptrROT && (scope & ~Activation::UserScope))
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
					if (!(pInfo->m_flags & Activation::MultipleRegistration) || pInfo->m_flags == flags)
						throw IAlreadyExistsException::Create(string_t::constant("The OID {0} has already been registered") % oid);
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

void User::RunningObjectTable::GetObject(const any_t& oid, const guid_t& iid, IObject*& pObject, bool_t remote)
{
	ObjectPtr<IObject> ptrObject;

	// Compose search flags
	Activation::RegisterFlags_t search_flags = Activation::PublicScope;
	if (remote && !m_ptrROT)
		search_flags = Activation::PublicScope | Activation::ExternalPublic;

	OOBase::Stack<uint32_t,OOBase::LocalAllocator> revoke_list;
	string_t strOid = oid.cast<string_t>();

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	for (size_t i=m_mapObjectsByOid.find_first(strOid); i<m_mapObjectsByOid.size() && *m_mapObjectsByOid.key_at(i)==strOid;++i)
	{
		Info* pInfo = m_mapObjectsByCookie.find(*m_mapObjectsByOid.at(i));
		if (pInfo && (!remote || (pInfo->m_flags & search_flags)))
		{
			// Check its still alive...
			if (!Omega::Remoting::IsAlive(pInfo->m_ptrObject))
			{
				revoke_list.push(*m_mapObjectsByOid.at(i));
			}
			else
			{
				if (iid == OMEGA_GUIDOF(IObject))
					ptrObject = pInfo->m_ptrObject;
				else
				{
					ptrObject = pInfo->m_ptrObject->QueryInterface(iid);
					if (!ptrObject)
						throw OOCore_INotFoundException_MissingIID(iid);
				}

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
		pObject = ptrObject.Detach();
	else if (m_ptrROT)
	{
		// Route to global rot
		m_ptrROT->GetObject(oid,iid,pObject,remote);
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
