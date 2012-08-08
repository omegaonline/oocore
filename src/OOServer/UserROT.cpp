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

User::RunningObjectTable::RunningObjectTable() :
		m_notify_cookie(0),
		m_mapObjectsByCookie(1),
		m_mapNotify(1)
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
	if (!pObject)
		throw IInternalException::Create(string_t::constant("NULL object"),"IRunningObjectTable::RegisterObject");

	// Check for user registration
	Activation::RegisterFlags_t scope = (flags & 0xF);
	if (!scope)
		throw IInternalException::Create(string_t::constant("Invalid flags"),"IRunningObjectTable::RegisterObject");

	// Create a new info struct
	Info info;
	info.m_oid = oid.cast<string_t>();
	info.m_flags = flags;
	info.m_ptrObject = pObject;
	info.m_ptrObject.AddRef();
	info.m_rot_cookie = 0;
	info.m_source = 0;

	// Check for public registration...
	if (m_ptrROT && (scope & ~Activation::UserScope))
	{
		// Register in sandbox ROT
		info.m_rot_cookie = m_ptrROT->RegisterObject(oid,pObject,flags);
	}

	ObjectPtr<Remoting::ICallContext> ptrCC = Remoting::GetCallContext();
	if (ptrCC)
		info.m_source = ptrCC->SourceId();

	OOBase::Stack<uint32_t,OOBase::LocalAllocator> revoke_list;
	uint32_t nCookie = 0;

	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		// Check if we have someone registered already
		for (size_t i=m_mapObjectsByOid.find_first(info.m_oid); i<m_mapObjectsByOid.size() && *m_mapObjectsByOid.key_at(i)==info.m_oid; ++i)
		{
			// Check its still alive...
			Info* pInfo = m_mapObjectsByCookie.find(*m_mapObjectsByOid.at(i));
			if (pInfo)
			{
				if (!Omega::Remoting::IsAlive(pInfo->m_ptrObject))
					revoke_list.push(*m_mapObjectsByOid.at(i));
				else
				{
					if (!(pInfo->m_flags & Activation::MultipleRegistration) || pInfo->m_flags == flags)
						throw IAlreadyExistsException::Create(string_t::constant("The OID {0} has already been registered") % oid);
				}
			}
		}

		int err = m_mapObjectsByCookie.insert(info,nCookie,1,0xFFFFFFFF);
		if (err == 0)
		{
			err = m_mapObjectsByOid.insert(info.m_oid,nCookie);
			if (err != 0)
				m_mapObjectsByCookie.remove(nCookie);
		}
		if (err != 0)
			OMEGA_THROW(err);
	}
	catch (...)
	{
		if (info.m_rot_cookie)
			m_ptrROT->RevokeObject(info.m_rot_cookie);

		throw;
	}

	// Revoke the revoke_list BEFORE we notify of the new entry
	for (uint32_t i = 0;revoke_list.pop(&i);)
		RevokeObject_i(i,0);

	if (!info.m_rot_cookie)
		OnRegisterObject(info.m_oid,info.m_flags);

	return nCookie;
}

void User::RunningObjectTable::GetObject(const any_t& oid, const guid_t& iid, IObject*& pObject)
{
	// Compose search flags
	Remoting::MarshalFlags_t flags = Remoting::Same;
	ObjectPtr<Remoting::ICallContext> ptrCC = Remoting::GetCallContext();
	if (ptrCC)
		flags = ptrCC->SourceType();

	Activation::RegisterFlags_t search_flags = Activation::PublicScope;
	if (flags == Remoting::RemoteMachine && !m_ptrROT)
		search_flags = Activation::ExternalPublic;

	OOBase::Stack<uint32_t,OOBase::LocalAllocator> revoke_list;
	string_t strOid = oid.cast<string_t>();

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	ObjectPtr<IObject> ptrObject;
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
		m_ptrROT->GetObject(oid,iid,pObject);
	}
}

void User::RunningObjectTable::RevokeObject_i(uint32_t cookie, uint32_t src_id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	Info* pInfo = m_mapObjectsByCookie.find(cookie);
	if (pInfo && (!src_id || pInfo->m_source == src_id))
	{
		Info info = *pInfo;

		for (size_t i=m_mapObjectsByOid.find_first(info.m_oid); i<m_mapObjectsByOid.size() && *m_mapObjectsByOid.key_at(i)==info.m_oid; ++i)
		{
			if (*m_mapObjectsByOid.at(i) == cookie)
				m_mapObjectsByOid.remove_at(i);
		}
		m_mapObjectsByCookie.remove(cookie);

		guard.release();

		if (info.m_rot_cookie)
			m_ptrROT->RevokeObject(info.m_rot_cookie);
		else
			OnRevokeObject(info.m_oid,info.m_flags);
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

void User::RunningObjectTable::OnRegisterObject(const any_t& oid, Activation::RegisterFlags_t flags)
{
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	OOBase::Stack<uint32_t> stackRemove;

	for (size_t pos = m_mapNotify.begin(); pos != m_mapNotify.npos; pos = m_mapNotify.next(pos))
	{
		try
		{
			ObjectPtr<Activation::IRunningObjectTableNotify> ptrNotify = *m_mapNotify.at(pos);
			if (!ptrNotify || !Remoting::IsAlive(ptrNotify))
				stackRemove.push(*m_mapNotify.key_at(pos));
			else
				ptrNotify->OnRegisterObject(oid,flags);
		}
		catch (IException* pE)
		{
			pE->Release();
		}
	}

	read_guard.release();

	if (!stackRemove.empty())
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		for (uint32_t i = 0;stackRemove.pop(&i);)
			m_mapNotify.remove(i,NULL);
	}
}

void User::RunningObjectTable::OnRevokeObject(const any_t& oid, Activation::RegisterFlags_t flags)
{
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	OOBase::Stack<uint32_t> stackRemove;

	for (size_t pos = m_mapNotify.begin(); pos != m_mapNotify.npos; pos = m_mapNotify.next(pos))
	{
		try
		{
			ObjectPtr<Activation::IRunningObjectTableNotify> ptrNotify = *m_mapNotify.at(pos);
			if (!ptrNotify || !Remoting::IsAlive(ptrNotify))
				stackRemove.push(*m_mapNotify.key_at(pos));
			else
				ptrNotify->OnRevokeObject(oid,flags);
		}
		catch (IException* pE)
		{
			pE->Release();
		}
	}

	read_guard.release();

	if (!stackRemove.empty())
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		for (uint32_t i = 0;stackRemove.pop(&i);)
			m_mapNotify.remove(i,NULL);
	}
}

uint32_t User::RunningObjectTable::RegisterNotify(const guid_t& iid, IObject* pObject)
{
	uint32_t nCookie = 0;

	if (iid == OMEGA_GUIDOF(Activation::IRunningObjectTableNotify) && pObject)
	{
		ObjectPtr<Activation::IRunningObjectTableNotify> ptrNotify(static_cast<Activation::IRunningObjectTableNotify*>(pObject));
		ptrNotify.AddRef();

		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		int err = m_mapNotify.insert(ptrNotify,nCookie,1,0xFFFFFFFF);
		if (err)
			OMEGA_THROW(err);
	}

	return nCookie;
}

void User::RunningObjectTable::UnregisterNotify(const guid_t& iid, uint32_t cookie)
{
	if (iid == OMEGA_GUIDOF(Activation::IRunningObjectTableNotify) && cookie)
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		m_mapNotify.remove(cookie,NULL);
	}
}

Notify::INotifier::iid_list_t User::RunningObjectTable::ListNotifyInterfaces()
{
	Notify::INotifier::iid_list_t list;
	list.push_back(OMEGA_GUIDOF(Activation::IRunningObjectTableNotify));
	return list;
}
