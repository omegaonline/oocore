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
#include "IPS.h"

using namespace Omega;
using namespace OTL;

namespace
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

void DuplicateRegistrationException::Throw(const any_t& oid)
{
	ObjectImpl<DuplicateRegistrationException>* pRE = ObjectImpl<DuplicateRegistrationException>::CreateInstance();
	pRE->m_strDesc = L"Duplicate registration of oid {0} in running object table.";
	pRE->m_strDesc %= oid;
	pRE->m_oid = oid;
	throw static_cast<IDuplicateRegistrationException*>(pRE);
}

ObjectPtr<OOCore::IInterProcessService> OOCore::GetInterProcessService()
{
	ObjectPtr<Activation::IRunningObjectTable> ptrROT;
	ptrROT.Attach(Activation::IRunningObjectTable::GetRunningObjectTable());

	IObject* pIPS = 0;
	ptrROT->GetObject(OID_InterProcessService,Activation::ProcessLocal,OMEGA_GUIDOF(IInterProcessService),pIPS);
	if (!pIPS)
		throw IInternalException::Create("Omega::Initialize not called","OOCore");
		
	ObjectPtr<OOCore::IInterProcessService> ptrIPS;
	ptrIPS.Attach(static_cast<IInterProcessService*>(pIPS));
	return ptrIPS;
}

bool OOCore::HostedByOOServer()
{
	static bool bChecked = false;
	static bool bHosted = false;

	if (!bChecked)
	{
		// If the InterProcessService has a proxy, then we are not hosted by OOServer.exe
		ObjectPtr<IInterProcessService> ptrIPS = OOCore::GetInterProcessService();

		ObjectPtr<System::Internal::ISafeProxy> ptrSProxy(ptrIPS);
		if (ptrSProxy)
		{
			System::Internal::auto_safe_shim shim = ptrSProxy->GetShim(OMEGA_GUIDOF(IObject));
			if (!shim || !static_cast<const System::Internal::IObject_Safe_VTable*>(shim->m_vtable)->pfnGetWireProxy_Safe)
			{
				bHosted = !ptrIPS->IsStandalone();
			}
		}

		bChecked = true;
	}

	return bHosted;
}

OOCore::ServiceManager::ServiceManager() : m_nNextCookie(1)
{
}

OOCore::ServiceManager::~ServiceManager()
{
	// Because the DLL can be deleted without close being called...
	for (std::map<uint32_t,Info>::iterator i=m_mapServicesByCookie.begin(); i!=m_mapServicesByCookie.end(); ++i)
	{
		// Just detach and leak every object...
		i->second.m_ptrObject.Detach();
	}
}

void OOCore::ServiceManager::close()
{
	try
	{
		std::list<uint32_t> listCookies;

		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		for (std::map<uint32_t,Info>::iterator i=m_mapServicesByCookie.begin(); i!=m_mapServicesByCookie.end(); ++i)
		{
			listCookies.push_back(i->first);
		}

		guard.release();

		for (std::list<uint32_t>::iterator i=listCookies.begin(); i!=listCookies.end(); ++i)
		{
			RevokeObject(*i);
		}
	}
	catch (...)
	{
	}

	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	assert(m_mapServicesByCookie.empty());
	assert(m_mapServicesByOid.empty());
}

uint32_t OOCore::ServiceManager::RegisterObject(const any_t& oid, IObject* pObject, Activation::RegisterFlags_t flags)
{
	ObjectPtr<Activation::IRunningObjectTable> ptrROT;
	uint32_t rot_cookie = 0;

	// Check for user registration
	if (flags & (Activation::UserLocal | Activation::MachineLocal | Activation::Anywhere))
	{
		// Register in ROT
		ptrROT.Attach(Activation::IRunningObjectTable::GetRunningObjectTable());
		if (ptrROT)
		{
			rot_cookie = ptrROT->RegisterObject(oid,pObject,flags & ~Activation::ProcessLocal);
		}
	}
	
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		// Check if we have someone registered already
		string_t strOid = oid.cast<string_t>();
		for (std::multimap<string_t,std::map<uint32_t,Info>::iterator>::iterator i=m_mapServicesByOid.find(strOid); i!=m_mapServicesByOid.end() && i->first==strOid; ++i)
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
		uint32_t nCookie = m_nNextCookie++;
		while (nCookie==0 && m_mapServicesByCookie.find(nCookie) != m_mapServicesByCookie.end())
		{
			nCookie = m_nNextCookie++;
		}

		std::pair<std::map<uint32_t,Info>::iterator,bool> p = m_mapServicesByCookie.insert(std::map<uint32_t,Info>::value_type(nCookie,info));
		assert(p.second);

		m_mapServicesByOid.insert(std::multimap<string_t,std::map<uint32_t,Info>::iterator>::value_type(strOid,p.first));

		guard.release();

		// This forces the detection, so cleanup succeeds
		OOCore::HostedByOOServer();

		return nCookie;
	}
	catch (...)
	{
		if (rot_cookie && ptrROT)
			ptrROT->RevokeObject(rot_cookie);

		throw;
	}
}

void OOCore::ServiceManager::GetObject(const any_t& oid, Activation::RegisterFlags_t flags, const guid_t& iid, IObject*& pObject)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	bool bDead = false;
	string_t strOid = oid.cast<string_t>();
	for (std::multimap<string_t,std::map<uint32_t,Info>::iterator>::iterator i=m_mapServicesByOid.find(strOid); i!=m_mapServicesByOid.end() && i->first==strOid;++i)
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

		std::multimap<string_t,std::map<uint32_t,Info>::iterator>::iterator i=m_mapServicesByOid.find(strOid);
		
		// Remove it, its dead
		m_mapServicesByCookie.erase(i->second);

		while (i!=m_mapServicesByOid.end() && i->first==strOid)
			m_mapServicesByOid.erase(i++);
	}

	FIX ME!!!
	ObjectPtr<Activation::IRunningObjectTable> ptrROT;
	ptrROT.Attach(Activation::IRunningObjectTable::GetRunningObjectTable());

	if (ptrROT && (flags & (Activation::UserLocal | Activation::MachineLocal | Activation::Anywhere)))
	{
		// Route to global rot
		ptrROT->GetObject(oid,flags,iid,pObject);
	}
}

void OOCore::ServiceManager::RevokeObject(uint32_t cookie)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	std::map<uint32_t,Info>::iterator i = m_mapServicesByCookie.find(cookie);
	if (i != m_mapServicesByCookie.end())
	{
		uint32_t rot_cookie = i->second.m_rot_cookie;

		for (std::multimap<string_t,std::map<uint32_t,Info>::iterator>::iterator j=m_mapServicesByOid.find(i->second.m_oid); j!=m_mapServicesByOid.end() && j->first==i->second.m_oid; ++j)
		{
			if (j->second->first == cookie)
			{
				m_mapServicesByOid.erase(j);
				break;
			}
		}
		m_mapServicesByCookie.erase(i);

		guard.release();

		if (rot_cookie)
		{
			// Revoke from ROT
			ObjectPtr<Activation::IRunningObjectTable> ptrROT;
			ptrROT.Attach(Activation::IRunningObjectTable::GetRunningObjectTable());

			if (ptrROT)
				ptrROT->RevokeObject(rot_cookie);
		}
	}
}
