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
		static void Throw(const guid_t& oid, IException* pE = 0);

		BEGIN_INTERFACE_MAP(DuplicateRegistrationException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<Activation::IDuplicateRegistrationException>)
		END_INTERFACE_MAP()

	private:
		guid_t m_oid;

	// Activation::IDuplicateRegistrationException members
	public:
		guid_t GetObject()
		{
			return m_oid;
		}
	};
}

void DuplicateRegistrationException::Throw(const guid_t& oid, IException* pE)
{
	ObjectImpl<DuplicateRegistrationException>* pRE = ObjectImpl<DuplicateRegistrationException>::CreateInstance();
	pRE->m_ptrCause = pE;
	pRE->m_strDesc = L"Duplicate registration of oid {0} in running object table.";
	pRE->m_strDesc %= oid;
	pRE->m_oid = oid;
	throw static_cast<IDuplicateRegistrationException*>(pRE);
}

ObjectPtr<OOCore::IInterProcessService> OOCore::GetInterProcessService()
{
	try
	{
		ObjectPtr<IInterProcessService> ptrIPS;
		ptrIPS.Attach(static_cast<IInterProcessService*>(Activation::GetRegisteredObject(OID_InterProcessService,Activation::InProcess | Activation::DontLaunch,OMEGA_GUIDOF(IInterProcessService))));
		return ptrIPS;
	}
	catch (IException* pE2)
	{
		pE2->Release();
		throw ISystemException::Create(L"Omega::Initialize not called");
	}
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
	for (std::map<uint32_t,Info>::iterator i=m_mapServicesByCookie.begin();i!=m_mapServicesByCookie.end();++i)
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

		for (std::map<uint32_t,Info>::iterator i=m_mapServicesByCookie.begin();i!=m_mapServicesByCookie.end();++i)
		{
			listCookies.push_back(i->first);
		}

		guard.release();

		for (std::list<uint32_t>::iterator i=listCookies.begin();i!=listCookies.end();++i)
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

uint32_t OOCore::ServiceManager::RegisterObject(const guid_t& oid, IObject* pObject, Activation::Flags_t flags, Activation::RegisterFlags_t reg_flags)
{
	ObjectPtr<Activation::IRunningObjectTable> ptrROT;
	uint32_t rot_cookie = 0;
	if (flags & Activation::OutOfProcess)
	{
		// Register in ROT
		ptrROT.Attach(Activation::IRunningObjectTable::GetRunningObjectTable());
		if (ptrROT)
		{
			rot_cookie = ptrROT->Register(oid,pObject);
		}
	}

	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		// Remove any flags we don't care about...
		flags &= (Activation::RemoteActivation | Activation::InProcess | Activation::OutOfProcess);

		// Check if we have someone registered already
		for (std::multimap<guid_t,std::map<uint32_t,Info>::iterator>::iterator i=m_mapServicesByOid.find(oid);i!=m_mapServicesByOid.end() && i->first==oid;++i)
		{
			if (!(i->second->second.m_reg_flags & Activation::MultipleRegistration))
				DuplicateRegistrationException::Throw(oid);

			if (i->second->second.m_reg_flags == reg_flags)
				DuplicateRegistrationException::Throw(oid);
		}

		// Create a new cookie
		Info info;
		info.m_oid = oid;
		info.m_flags = flags;
		info.m_reg_flags = reg_flags;
		info.m_ptrObject = pObject;
		info.m_rot_cookie = rot_cookie;
		uint32_t nCookie = m_nNextCookie++;
		while (nCookie==0 && m_mapServicesByCookie.find(nCookie) != m_mapServicesByCookie.end())
		{
			nCookie = m_nNextCookie++;
		}

		std::pair<std::map<uint32_t,Info>::iterator,bool> p = m_mapServicesByCookie.insert(std::map<uint32_t,Info>::value_type(nCookie,info));
		assert(p.second);			

		m_mapServicesByOid.insert(std::multimap<guid_t,std::map<uint32_t,Info>::iterator>::value_type(oid,p.first));

		return nCookie;
	}
	catch (...)
	{
		if (rot_cookie && ptrROT)
			ptrROT->Revoke(rot_cookie);

		throw;
	}
}

IObject* OOCore::ServiceManager::GetObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid)
{
	// Remove any flags we don't care about...
	flags &= (Activation::RemoteActivation | Activation::InProcess | Activation::OutOfProcess);

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	for (std::multimap<guid_t,std::map<uint32_t,Info>::iterator>::iterator i=m_mapServicesByOid.find(oid);i!=m_mapServicesByOid.end() && i->first==oid;++i)
	{
		if ((i->second->second.m_flags & flags) == i->second->second.m_flags)
			return i->second->second.m_ptrObject->QueryInterface(iid);
	}

	// No, didn't find it
	return 0;
}

void OOCore::ServiceManager::RevokeObject(uint32_t cookie)
{
	uint32_t rot_cookie = 0;

	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	std::map<uint32_t,Info>::iterator i = m_mapServicesByCookie.find(cookie);
	if (i != m_mapServicesByCookie.end())
	{
		rot_cookie = i->second.m_rot_cookie;

		for (std::multimap<guid_t,std::map<uint32_t,Info>::iterator>::iterator j=m_mapServicesByOid.find(i->second.m_oid);j!=m_mapServicesByOid.end() && j->first==i->second.m_oid;++j)
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
				ptrROT->Revoke(rot_cookie);
		}
	}
}
