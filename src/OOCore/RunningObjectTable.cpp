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

#include "OOCore_precomp.h"

#include "Activation.h"

using namespace Omega;
using namespace OTL;

OOCore::LocalROT::LocalROT() :
		m_notify_cookie(0),
		m_mapServicesByCookie(1),
		m_mapNotify(1)
{
}

void OOCore::LocalROT::SetUpstreamROT(Omega::Activation::IRunningObjectTable* pROT)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	if (!m_notify_cookie && pROT)
	{
		m_ptrROT = pROT;
		m_ptrROT.AddRef();

		guard.release();

		ObjectPtr<Notify::INotifier> ptrNotify = static_cast<Notify::INotifier*>(pROT->QueryInterface(OMEGA_GUIDOF(Notify::INotifier)));
		uint32_t cookie = ptrNotify->RegisterNotify(OMEGA_GUIDOF(Activation::IRunningObjectTableNotify),static_cast<Activation::IRunningObjectTableNotify*>(this));
		if (cookie)
		{
			guard.acquire();

			m_notify_cookie = cookie;
		}
	}
	else if (m_notify_cookie && m_ptrROT)
	{
		ObjectPtr<Activation::IRunningObjectTable> ptrROT = m_ptrROT;
		m_ptrROT.Release();

		uint32_t cookie = m_notify_cookie;
		m_notify_cookie = 0;

		guard.release();

		ObjectPtr<Notify::INotifier> ptrNotify = ptrROT.QueryInterface<Notify::INotifier>();
		if (ptrNotify)
			ptrNotify->UnregisterNotify(OMEGA_GUIDOF(Activation::IRunningObjectTableNotify),cookie);
	}
}

ObjectPtr<Activation::IRunningObjectTable> OOCore::LocalROT::GetUpstreamROT(bool bThrow)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	if (bThrow && !m_ptrROT)
		throw IInternalException::Create(OOCore::get_text("Omega::Initialize not called"),"OOCore");

	return m_ptrROT;
}

uint32_t OOCore::LocalROT::RegisterObject(const any_t& oid, IObject* pObject, Activation::RegisterFlags_t flags)
{
	if (!pObject)
		throw IInternalException::Create(OOCore::get_text("NULL object"),"IRunningObjectTable::RegisterObject");

	// Check for user registration
	Activation::RegisterFlags_t scope = (flags & 0xF);
	if (!scope)
		throw IInternalException::Create(OOCore::get_text("Invalid flags"),"IRunningObjectTable::RegisterObject");

	// Create a new info struct
	Info info;
	info.m_oid = oid.cast<string_t>();
	info.m_flags = flags;
	info.m_ptrObject = pObject;
	info.m_ptrObject.AddRef();
	info.m_rot_cookie = 0;

	if (scope & ~Activation::ProcessScope)
	{
		// Register in ROT
		info.m_rot_cookie = GetUpstreamROT(true)->RegisterObject(oid,pObject,flags);
	}

	OOBase::StackAllocator<128> allocator;
	OOBase::Vector<uint32_t,OOBase::AllocatorInstance> revoke_list(allocator);
	uint32_t nCookie = 0;

	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		// Check if we have someone registered already
		for (size_t i=m_mapServicesByOid.find_first(info.m_oid); i<m_mapServicesByOid.size() && *m_mapServicesByOid.key_at(i)==info.m_oid; ++i)
		{
			Info* pInfo = m_mapServicesByCookie.find(*m_mapServicesByOid.at(i));
			if (pInfo)
			{
				// Check its still alive...
				if (!Remoting::IsAlive(pInfo->m_ptrObject))
					revoke_list.push_back(*m_mapServicesByOid.at(i));
				else
				{
					if (!(pInfo->m_flags & Activation::MultipleRegistration) || pInfo->m_flags == flags)
						throw IAlreadyExistsException::Create(OOCore::get_text("The OID {0} has already been registered") % oid);
				}
			}
		}

		int err = m_mapServicesByCookie.insert(info,nCookie,1,0xFFFFFFFF);
		if (err == 0)
		{
			err = m_mapServicesByOid.insert(info.m_oid,nCookie);
			if (err != 0)
				m_mapServicesByCookie.remove(nCookie);
		}
		if (err)
			OMEGA_THROW(err);
	}
	catch (...)
	{
		if (info.m_rot_cookie)
			GetUpstreamROT(true)->RevokeObject(info.m_rot_cookie);

		throw;
	}
		
	// Revoke the revoke_list BEFORE we notify of the new entry
	for (uint32_t i = 0;revoke_list.pop_back(&i);)
		RevokeObject(i);

	if (!info.m_rot_cookie)
		OnRegisterObject(info.m_oid,info.m_flags);
		
	return nCookie;
}

void OOCore::LocalROT::GetObject(const any_t& oid, const guid_t& iid, IObject*& pObject)
{
	OOBase::StackAllocator<128> allocator;
	OOBase::Vector<uint32_t,OOBase::AllocatorInstance> revoke_list(allocator);
	string_t strOid = oid.cast<string_t>();

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);
	
	if (oid == OOCore::OID_InterProcessService)
	{
		pObject = OTL::GetModule()->GetIPS()->QueryInterface(iid);
		if (!pObject)
			throw OOCore_INotFoundException_MissingIID(iid);

		return;
	}

	ObjectPtr<IObject> ptrObject;
	for (size_t i=m_mapServicesByOid.find_first(strOid); i<m_mapServicesByOid.size() && *m_mapServicesByOid.key_at(i)==strOid; ++i)
	{
		Info* pInfo = m_mapServicesByCookie.find(*m_mapServicesByOid.at(i));
		if (pInfo)
		{
			// Check its still alive...
			if (!Remoting::IsAlive(pInfo->m_ptrObject))
			{
				int err = revoke_list.push_back(*m_mapServicesByOid.at(i));
				if (err != 0)
					OMEGA_THROW(err);
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
					int err = revoke_list.push_back(*m_mapServicesByOid.at(i));
					if (err != 0)
						OMEGA_THROW(err);
				}
				break;
			}
		}
	}

	guard.release();

	// Revoke the revoke_list
	for (uint32_t i = 0;revoke_list.pop_back(&i);)
		RevokeObject(i);
	
	// If we have an object, get out now
	if (ptrObject)
		pObject = ptrObject.Detach();
	else
	{
		// Route to global rot
		ObjectPtr<Activation::IRunningObjectTable> ptrROT = GetUpstreamROT(false);
		if (ptrROT)
			ptrROT->GetObject(oid,iid,pObject);
	}
}

void OOCore::LocalROT::RevokeObject(uint32_t cookie)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);
	
	Info info;
	if (m_mapServicesByCookie.remove(cookie,&info))
	{
		for (size_t i=m_mapServicesByOid.find_first(info.m_oid); i<m_mapServicesByOid.size() && *m_mapServicesByOid.key_at(i)==info.m_oid; ++i)
		{
			if (*m_mapServicesByOid.at(i) == cookie)
				m_mapServicesByOid.remove_at(i);
		}

		guard.release();

		if (info.m_rot_cookie)
		{
			// Revoke from ROT
			ObjectPtr<Activation::IRunningObjectTable> ptrROT = GetUpstreamROT(false);
			if (ptrROT)
				ptrROT->RevokeObject(info.m_rot_cookie);
		}
		else
			OnRevokeObject(info.m_oid,info.m_flags);
	}
}

void OOCore::LocalROT::OnRegisterObject(const any_t& oid, Activation::RegisterFlags_t flags)
{
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	OOBase::Vector<uint32_t> stackRemove;

	for (size_t pos = m_mapNotify.begin(); pos != m_mapNotify.npos; pos = m_mapNotify.next(pos))
	{
		try
		{
			ObjectPtr<Activation::IRunningObjectTableNotify> ptrNotify = *m_mapNotify.at(pos);
			if (!ptrNotify || !Remoting::IsAlive(ptrNotify))
				stackRemove.push_back(*m_mapNotify.key_at(pos));
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

		for (uint32_t i = 0;stackRemove.pop_back(&i);)
			m_mapNotify.remove(i,NULL);
	}
}

void OOCore::LocalROT::OnRevokeObject(const any_t& oid, Activation::RegisterFlags_t flags)
{
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	OOBase::Vector<uint32_t> stackRemove;

	for (size_t pos = m_mapNotify.begin(); pos != m_mapNotify.npos; pos = m_mapNotify.next(pos))
	{
		try
		{
			ObjectPtr<Activation::IRunningObjectTableNotify> ptrNotify = *m_mapNotify.at(pos);
			if (!ptrNotify || !Remoting::IsAlive(ptrNotify))
				stackRemove.push_back(*m_mapNotify.key_at(pos));
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

		for (uint32_t i = 0;stackRemove.pop_back(&i);)
			m_mapNotify.remove(i,NULL);
	}
}

uint32_t OOCore::LocalROT::RegisterNotify(const guid_t& iid, IObject* pObject)
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

void OOCore::LocalROT::UnregisterNotify(const guid_t& iid, uint32_t cookie)
{
	if (iid == OMEGA_GUIDOF(Activation::IRunningObjectTableNotify) && cookie)
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		m_mapNotify.remove(cookie,NULL);
	}
}

Notify::INotifier::iid_list_t OOCore::LocalROT::ListNotifyInterfaces()
{
	Notify::INotifier::iid_list_t list;
	list.push_back(OMEGA_GUIDOF(Activation::IRunningObjectTableNotify));
	return list;
}
