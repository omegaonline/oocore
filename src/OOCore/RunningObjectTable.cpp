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
	pRE->m_strDesc = L"Duplicate registration of oid {0} in running object table." % oid;
	pRE->m_oid = oid;
	throw static_cast<IDuplicateRegistrationException*>(pRE);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(uint32_t,OOCore_RegisterIPS,1,((in),IObject*,pIPS))
{
	// Get the zero cmpt service manager...
	ObjectPtr<Activation::IRunningObjectTable> ptrROT = SingletonObjectImpl<OOCore::ServiceManager>::CreateInstance();
	uint32_t nCookie = ptrROT->RegisterObject(OOCore::OID_InterProcessService,pIPS,Activation::ProcessScope | Activation::MultipleUse);
	
	// This forces the detection, so cleanup succeeds
	OOCore::HostedByOOServer();
	
	return nCookie;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_RevokeIPS,1,((in),uint32_t,nCookie))
{
	// Get the zero cmpt service manager...
	if (nCookie)
	{
		ObjectPtr<Activation::IRunningObjectTable> ptrROT = SingletonObjectImpl<OOCore::ServiceManager>::CreateInstance();
		ptrROT->RevokeObject(nCookie);
	}
}

OOCore::IInterProcessService* OOCore::GetInterProcessService()
{
	ObjectPtr<SingletonObjectImpl<OOCore::ServiceManager> > ptrSM = SingletonObjectImpl<OOCore::ServiceManager>::CreateInstance();
	return ptrSM->GetIPS();
}

bool OOCore::HostedByOOServer()
{
	static bool bChecked = false;
	static bool bHosted = false;

	if (!bChecked)
	{
		// If the InterProcessService has a proxy, then we are not hosted by OOServer.exe
		ObjectPtr<IInterProcessService> ptrIPS = GetInterProcessService();
		
		ObjectPtr<System::Internal::ISafeProxy> ptrSProxy = ptrIPS.QueryInterface<System::Internal::ISafeProxy>();
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

OOCore::ServiceManager::ServiceManager() : m_mapServicesByCookie(1)
{
}

OOCore::ServiceManager::~ServiceManager()
{
	try
	{
		// Because the DLL can be deleted without close being called...
		Info info;
		while (m_mapServicesByCookie.pop(NULL,&info))
		{
			// Just detach and leak every object...
			info.m_ptrObject.Detach();
		}
	}
	catch (IException* pE)
	{
		pE->Release();
	}
}

OOCore::IInterProcessService* OOCore::ServiceManager::GetIPS()
{
	IObject* pIPS = NULL;
	GetObject(OID_InterProcessService,Activation::ProcessScope,OMEGA_GUIDOF(IInterProcessService),pIPS);
	
	if (!pIPS)
		throw IInternalException::Create("Omega::Initialize not called","OOCore");

	return static_cast<IInterProcessService*>(pIPS);
}

uint32_t OOCore::ServiceManager::RegisterObject(const any_t& oid, IObject* pObject, Activation::RegisterFlags_t flags)
{
	ObjectPtr<Activation::IRunningObjectTable> ptrROT;
	uint32_t rot_cookie = 0;

	// Check for user registration
	if (flags & ~Activation::ProcessScope)
	{
		// Register in ROT
		ObjectPtr<IInterProcessService> ptrIPS = GetIPS();
		if (ptrIPS)
		{
			ptrROT = ptrIPS->GetRunningObjectTable();
			if (ptrROT)
			{
				rot_cookie = ptrROT->RegisterObject(oid,pObject,static_cast<Activation::RegisterFlags_t>(flags & ~Activation::ProcessScope));
			}
		}
	}

	try
	{
		OOBase::Stack<uint32_t,OOBase::LocalAllocator> revoke_list;
		string_t strOid = oid.cast<string_t>();

		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		// Check if we have someone registered already
		for (size_t i=m_mapServicesByOid.find_first(strOid); i<m_mapServicesByOid.size() && *m_mapServicesByOid.key_at(i)==strOid; ++i)
		{
			Info* pInfo = m_mapServicesByCookie.find(*m_mapServicesByOid.at(i));
			if (pInfo)
			{
				// Check its still alive...
				if (!Omega::Remoting::IsAlive(pInfo->m_ptrObject))
				{
					int err = revoke_list.push(*m_mapServicesByOid.at(i));
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
		info.m_ptrObject->AddRef();
		info.m_rot_cookie = rot_cookie;

		uint32_t nCookie = 0;
		int err = m_mapServicesByCookie.insert(info,nCookie,1,0xFFFFFFFF);
		if (err == 0)
		{
			err = m_mapServicesByOid.insert(strOid,nCookie);
			if (err != 0)
				m_mapServicesByCookie.remove(nCookie);
		}
		if (err != 0)
			OMEGA_THROW(err);
		
		guard.release();

		// Revoke the revoke_list
		for (uint32_t i = 0;revoke_list.pop(&i);)
			RevokeObject(i);
		
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
	ObjectPtr<IObject> ptrObject;

	// Strip off the option flags
	Activation::RegisterFlags_t search_flags = flags & 0xF;

	OOBase::Stack<uint32_t,OOBase::LocalAllocator> revoke_list;
	string_t strOid = oid.cast<string_t>();

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);
	
	for (size_t i=m_mapServicesByOid.find_first(strOid); i<m_mapServicesByOid.size() && *m_mapServicesByOid.key_at(i)==strOid; ++i)
	{
		Info* pInfo = m_mapServicesByCookie.find(*m_mapServicesByOid.at(i));
		if (pInfo && (pInfo->m_flags & search_flags))
		{
			// Check its still alive...
			if (!Omega::Remoting::IsAlive(pInfo->m_ptrObject))
			{
				int err = revoke_list.push(*m_mapServicesByOid.at(i));
				if (err != 0)
					OMEGA_THROW(err);
			}
			else
			{
				ptrObject = pInfo->m_ptrObject->QueryInterface(iid);
				if (!ptrObject)
					throw INoInterfaceException::Create(iid);

				// Remove the entry if Activation::SingleUse
				if (pInfo->m_flags & Activation::SingleUse)
				{
					int err = revoke_list.push(*m_mapServicesByOid.at(i));
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
		RevokeObject(i);
	
	// If we have an object, get out now
	if (ptrObject)
	{
		pObject = ptrObject.AddRef();
		return;
	}

	if (flags & ~Activation::ProcessScope)
	{
		ObjectPtr<IInterProcessService> ptrIPS = GetIPS();
		if (ptrIPS)
		{
			ObjectPtr<Activation::IRunningObjectTable> ptrROT = ptrIPS->GetRunningObjectTable();
			if (ptrROT)
			{
				// Route to global rot
				ptrROT->GetObject(oid,flags,iid,pObject);
			}
		}
	}
}

void OOCore::ServiceManager::RevokeObject(uint32_t cookie)
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
			ObjectPtr<IInterProcessService> ptrIPS = GetIPS();
			if (ptrIPS)
			{
				ObjectPtr<Activation::IRunningObjectTable> ptrROT = ptrIPS->GetRunningObjectTable();
				if (ptrROT)
					ptrROT->RevokeObject(info.m_rot_cookie);
			}
		}
	}
}

// {F67F5A41-BA32-48C9-BFD2-7B3701984DC8}
OMEGA_DEFINE_OID(Activation,OID_RunningObjectTable,"{F67F5A41-BA32-48C9-BFD2-7B3701984DC8}");

void OOCore::RunningObjectTableFactory::CreateInstance(IObject* pOuter, const guid_t& iid, IObject*& pObject)
{
	if (pOuter)
		throw Omega::Activation::INoAggregationException::Create(Activation::OID_RunningObjectTable);

	ObjectPtr<IInterProcessService> ptrIPS = OOCore::GetInterProcessService();
	if (ptrIPS)
	{
		ObjectPtr<Activation::IRunningObjectTable> ptrROT = ptrIPS->GetRunningObjectTable();
		if (ptrROT)
			pObject = ptrROT->QueryInterface(iid);
	}
}
