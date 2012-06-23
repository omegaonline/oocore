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
#include "WireProxy.h"
#include "Channel.h"
#include "Exception.h"
#include "Activation.h"
#include "Compartment.h"

namespace OTL
{
	// The following is an expansion of BEGIN_LIBRARY_OBJECT_MAP
	// We don't use the macro as we override some behaviours
	namespace Module
	{
		class OOCore_LibraryModuleImpl : public LibraryModule
		{
		public:
			void RegisterObjectFactories(Omega::Activation::IRunningObjectTable* pROT);
			void UnregisterObjectFactories(Omega::Activation::IRunningObjectTable* pROT);

		private:
			ModuleBase::CreatorEntry* getCreatorEntries()
			{
				static ModuleBase::CreatorEntry CreatorEntries[] =
				{
					OBJECT_MAP_ENTRY(OOCore::CDRMessageMarshalFactory)
					OBJECT_MAP_ENTRY(OOCore::ChannelMarshalFactory)
					OBJECT_MAP_ENTRY(OOCore::ProxyMarshalFactory)
					OBJECT_MAP_FACTORY_ENTRY(OOCore::RunningObjectTableFactory)
					OBJECT_MAP_FACTORY_ENTRY(OOCore::RunningObjectTableFactory_NoThrow)
					OBJECT_MAP_FACTORY_ENTRY(OOCore::RegistryFactory)
					OBJECT_MAP_FACTORY_ENTRY(OOCore::CompartmentFactory)
					OBJECT_MAP_ENTRY(OOCore::SystemExceptionMarshalFactoryImpl)
					OBJECT_MAP_ENTRY(OOCore::InternalExceptionMarshalFactoryImpl)
					OBJECT_MAP_ENTRY(OOCore::NotFoundExceptionMarshalFactoryImpl)
					OBJECT_MAP_ENTRY(OOCore::AccessDeniedExceptionMarshalFactoryImpl)
					OBJECT_MAP_ENTRY(OOCore::AlreadyExistsExceptionMarshalFactoryImpl)
					OBJECT_MAP_ENTRY(OOCore::TimeoutExceptionMarshalFactoryImpl)
					OBJECT_MAP_ENTRY(OOCore::ChannelClosedExceptionMarshalFactoryImpl)
					{ 0,0,0,0 }
				};
				return CreatorEntries;
			}
		};

		OMEGA_PRIVATE_FN_DECL(Module::OOCore_LibraryModuleImpl*,GetModule())
		{
			return Omega::Threading::Singleton<Module::OOCore_LibraryModuleImpl,Omega::Threading::ModuleDestructor<OOCore::DLL> >::instance();
		}

		OMEGA_PRIVATE_FN_DECL(ModuleBase*,GetModuleBase)()
		{
			return OMEGA_PRIVATE_FN_CALL(GetModule)();
		}
	}
}

using namespace Omega;
using namespace OTL;

void OOCore::RegisterObjects()
{
	// Register all our local class factories
	ObjectPtr<Activation::IRunningObjectTable> ptrROT = SingletonObjectImpl<OOCore::LocalROT>::CreateInstance();
	Module::OMEGA_PRIVATE_FN_CALL(GetModule)()->RegisterObjectFactories(ptrROT);
}

void OOCore::UnregisterObjects()
{
	// Unregister all our local class factories
	ObjectPtr<Activation::IRunningObjectTable> ptrROT = SingletonObjectImpl<OOCore::LocalROT>::CreateInstance();
	Module::OMEGA_PRIVATE_FN_CALL(GetModule)()->UnregisterObjectFactories(ptrROT);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_ServerInit,0,())
{
	OOCore::RegisterObjects();
}

namespace
{
	bool s_hosted_by_ooserver = false;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(uint32_t,OOCore_RegisterIPS,2,((in),Omega::IObject*,pIPS,(in),bool_t,hosted))
{
	// Get the zero cmpt service manager...
	ObjectPtr<Activation::IRunningObjectTable> ptrROT = SingletonObjectImpl<OOCore::LocalROT>::CreateInstance();
	uint32_t nCookie = ptrROT->RegisterObject(OOCore::OID_InterProcessService,pIPS,Activation::ProcessScope | Activation::MultipleUse);
	
	s_hosted_by_ooserver = hosted;

	return nCookie;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_RevokeIPS,1,((in),uint32_t,nCookie))
{
	// Get the zero compartment service manager...
	if (nCookie)
	{
		ObjectPtr<Activation::IRunningObjectTable> ptrROT = SingletonObjectImpl<OOCore::LocalROT>::CreateInstance();
		ptrROT->RevokeObject(nCookie);
	}
}

ObjectPtr<OOCore::IInterProcessService> OOCore::GetInterProcessService(bool bThrow)
{
	ObjectPtr<SingletonObjectImpl<OOCore::LocalROT> > ptrROT = SingletonObjectImpl<OOCore::LocalROT>::CreateInstance();
	return ptrROT->GetIPS(bThrow);
}

bool OOCore::HostedByOOServer()
{
	return s_hosted_by_ooserver;
}

OOCore::LocalROT::LocalROT() : m_mapServicesByCookie(1)
{
}

OOCore::LocalROT::~LocalROT()
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

ObjectPtr<OOCore::IInterProcessService> OOCore::LocalROT::GetIPS(bool bThrow)
{
	IObject* pIPS = NULL;
	GetObject(OID_InterProcessService,OMEGA_GUIDOF(IInterProcessService),pIPS,false);
	
	if (bThrow && !pIPS)
		throw IInternalException::Create("Omega::Initialize not called","OOCore");

	return static_cast<IInterProcessService*>(pIPS);
}

uint32_t OOCore::LocalROT::RegisterObject(const any_t& oid, IObject* pObject, Activation::RegisterFlags_t flags)
{
	ObjectPtr<Activation::IRunningObjectTable> ptrROT;
	uint32_t rot_cookie = 0;

	// Check for user registration
	Activation::RegisterFlags_t scope = (flags & 0xF);
	if (scope & ~Activation::ProcessScope)
	{
		// Register in ROT
		ptrROT = GetIPS(true)->GetRunningObjectTable();
		if (ptrROT)
			rot_cookie = ptrROT->RegisterObject(oid,pObject,flags);
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
					if (!(pInfo->m_flags & Activation::MultipleRegistration) || pInfo->m_flags == flags)
						throw IAlreadyExistsException::Create(OOCore::get_text("The OID {0} has already been registered") % oid);
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

void OOCore::LocalROT::GetObject(const any_t& oid, const guid_t& iid, IObject*& pObject, bool_t remote)
{
	ObjectPtr<IObject> ptrObject;

	OOBase::Stack<uint32_t,OOBase::LocalAllocator> revoke_list;
	string_t strOid = oid.cast<string_t>();

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);
	
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
		pObject = ptrObject.Detach();
	else if (oid != OID_InterProcessService)
	{
		// Route to global rot
		ObjectPtr<Activation::IRunningObjectTable> ptrROT = GetIPS(true)->GetRunningObjectTable();
		ptrROT->GetObject(oid,iid,pObject,remote);
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
			ObjectPtr<OOCore::IInterProcessService> ptrIPS = GetIPS(false);
			if (ptrIPS)
			{
				ObjectPtr<Activation::IRunningObjectTable> ptrROT = ptrIPS->GetRunningObjectTable();
				ptrROT->RevokeObject(info.m_rot_cookie);
			}
		}
	}
}

void OTL::Module::OOCore_LibraryModuleImpl::RegisterObjectFactories(Activation::IRunningObjectTable* pROT)
{
	CreatorEntry* g=getCreatorEntries();
	for (size_t i=0; g[i].pfnOid!=0; ++i)
	{
		ObjectPtr<Activation::IObjectFactory> ptrOF = static_cast<Activation::IObjectFactory*>(g[i].pfnCreate(OMEGA_GUIDOF(Activation::IObjectFactory)));

		g[i].cookie = pROT->RegisterObject(*(g[i].pfnOid)(),ptrOF,Activation::ProcessScope | Activation::MultipleUse);
	}
}

void OTL::Module::OOCore_LibraryModuleImpl::UnregisterObjectFactories(Activation::IRunningObjectTable* pROT)
{
	CreatorEntry* g=getCreatorEntries();
	for (size_t i=0; g[i].pfnOid!=0; ++i)
	{
		pROT->RevokeObject(g[i].cookie);
		g[i].cookie = 0;
	}
}

// {F67F5A41-BA32-48C9-BFD2-7B3701984DC8}
OMEGA_DEFINE_OID(Activation,OID_RunningObjectTable,"{F67F5A41-BA32-48C9-BFD2-7B3701984DC8}");

void OOCore::RunningObjectTableFactory::CreateInstance(const guid_t& iid, IObject*& pObject)
{
	ObjectPtr<Activation::IRunningObjectTable> ptrROT = OOCore::GetInterProcessService(true)->GetRunningObjectTable();
	pObject = ptrROT->QueryInterface(iid);
	if (!pObject)
		throw OOCore_INotFoundException_MissingIID(iid);
}

// {ADEA9DC0-9D82-9481-843C-CFBB8373F65E}
OMEGA_DEFINE_OID(Activation,OID_RunningObjectTable_NoThrow,"{ADEA9DC0-9D82-9481-843C-CFBB8373F65E}");

void OOCore::RunningObjectTableFactory_NoThrow::CreateInstance(const guid_t& iid, IObject*& pObject)
{
	ObjectPtr<OOCore::IInterProcessService> ptrIPS = OOCore::GetInterProcessService(false);
	if (!ptrIPS)
		pObject = NULL;
	else
	{
		ObjectPtr<Activation::IRunningObjectTable> ptrROT = ptrIPS->GetRunningObjectTable();
		pObject = ptrROT->QueryInterface(iid);
		if (!pObject)
			throw OOCore_INotFoundException_MissingIID(iid);
	}
}
