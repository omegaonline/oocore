///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
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

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	ObjectPtr<Remoting::IInterProcessService> GetInterProcessService();

	class OidNotFoundException :
		public ExceptionImpl<Activation::IOidNotFoundException>
	{
	public:
		guid_t m_oid;

		static void Throw(const guid_t& Oid, IException* pE = 0);

		BEGIN_INTERFACE_MAP(OidNotFoundException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<Activation::IOidNotFoundException>)
		END_INTERFACE_MAP()

	// Activation::IOidNotFoundException members
	public:
		guid_t GetMissingOid()
		{
			return m_oid;
		}
	};

	class NoAggregationException :
		public ExceptionImpl<Activation::INoAggregationException>
	{
	public:
		guid_t	m_oid;

		BEGIN_INTERFACE_MAP(NoAggregationException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<Activation::INoAggregationException>)
		END_INTERFACE_MAP()

	// Activation::INoAggregationException members
	public:
		guid_t GetFailingOid()
		{
			return m_oid;
		}
	};

	class LibraryNotFoundException :
		public ExceptionImpl<Activation::ILibraryNotFoundException>
	{
	public:
		static void Throw(const string_t& strName, IException* pE = 0);

		BEGIN_INTERFACE_MAP(LibraryNotFoundException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<Activation::ILibraryNotFoundException>)
		END_INTERFACE_MAP()

	private:
		string_t m_dll_name;

	// Activation::ILibraryNotFoundException members
	public:
		string_t GetLibraryName()
		{
			return m_dll_name;
		}
	};

	// The instance wide ServiceManager instance
	class ServiceManager
	{
	public:
		uint32_t RegisterObject(const guid_t& oid, IObject* pObject, Activation::Flags_t flags, Activation::RegisterFlags_t reg_flags);
		IObject* GetObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid);
		void RevokeObject(uint32_t cookie);

	private:
		friend class ACE_Singleton<ServiceManager, ACE_Thread_Mutex>;
				
		ServiceManager();
		ServiceManager(const ServiceManager&) {}
		ServiceManager& operator = (const ServiceManager&) { return *this; }

		ACE_RW_Thread_Mutex m_lock;
		uint32_t            m_nNextCookie;

		struct Info
		{
			guid_t                      m_oid;
			ObjectPtr<IObject>          m_ptrObject;
			Activation::Flags_t         m_flags;
			Activation::RegisterFlags_t m_reg_flags;
			uint32_t                    m_rot_cookie;
		};
		std::map<uint32_t,Info>                                 m_mapServicesByCookie;
		std::multimap<guid_t,std::map<uint32_t,Info>::iterator> m_mapServicesByOid;
	};
	typedef ACE_Singleton<ServiceManager, ACE_Thread_Mutex> SERVICE_MANAGER;

	IObject* LoadLibraryObject(const string_t& dll_name, const guid_t& oid, Activation::Flags_t flags, const guid_t& iid);
}

void OOCore::OidNotFoundException::Throw(const guid_t& oid, IException* pE)
{
	ObjectImpl<OOCore::OidNotFoundException>* pNew = ObjectImpl<OOCore::OidNotFoundException>::CreateInstance();
	pNew->m_strDesc = L"The identified object could not be found.";
	pNew->m_ptrCause = pE;
	pNew->m_oid = oid;
	throw static_cast<IOidNotFoundException*>(pNew);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::Activation::INoAggregationException*,Activation_INoAggregationException_Create,1,((in),const guid_t&,oid))
{
	ObjectImpl<OOCore::NoAggregationException>* pNew = ObjectImpl<OOCore::NoAggregationException>::CreateInstance();
	pNew->m_strDesc = L"Object does not support aggregation.";
	pNew->m_oid = oid;
	return pNew;
}

void OOCore::LibraryNotFoundException::Throw(const string_t& strName, IException* pE)
{
	ObjectImpl<OOCore::LibraryNotFoundException>* pRE = ObjectImpl<OOCore::LibraryNotFoundException>::CreateInstance();
	pRE->m_ptrCause = pE;
	pRE->m_strDesc = string_t::Format(L"Dynamic library '%ls' not found",strName.c_str());
	pRE->m_dll_name = strName;
	throw static_cast<ILibraryNotFoundException*>(pRE);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::uint32_t,Activation_RegisterObject,4,((in),const Omega::guid_t&,oid,(in),Omega::IObject*,pObject,(in),Omega::Activation::Flags_t,flags,(in),Omega::Activation::RegisterFlags_t,reg_flags))
{
	return OOCore::SERVICE_MANAGER::instance()->RegisterObject(oid,pObject,flags,reg_flags);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Activation_RevokeObject,1,((in),Omega::uint32_t,cookie))
{
	OOCore::SERVICE_MANAGER::instance()->RevokeObject(cookie);
}

OOCore::ServiceManager::ServiceManager() : m_nNextCookie(1)
{
}

uint32_t OOCore::ServiceManager::RegisterObject(const guid_t& oid, IObject* pObject, Activation::Flags_t flags, Activation::RegisterFlags_t reg_flags)
{
	ObjectPtr<Activation::IRunningObjectTable> ptrROT;
	uint32_t rot_cookie = 0;
	if (flags & Activation::OutOfProcess)
	{
		// Register in ROT
		ptrROT.Attach(Activation::IRunningObjectTable::GetRunningObjectTable());			

		rot_cookie = ptrROT->Register(oid,pObject);
	}

	try
	{
		OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		// Remove any flags we don't store...
		flags &= ~(Activation::DontLaunch);

		// Check if we have someone registered already
		for (std::multimap<guid_t,std::map<uint32_t,Info>::iterator>::iterator i=m_mapServicesByOid.find(oid);i!=m_mapServicesByOid.end() && i->first==oid;++i)
		{
			if (!(i->second->second.m_reg_flags & Activation::MultipleRegistration))
				OMEGA_THROW(EALREADY);

			if (i->second->second.m_reg_flags == reg_flags)
				OMEGA_THROW(EALREADY);
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
		if (!p.second)
			OMEGA_THROW(EALREADY);

		m_mapServicesByOid.insert(std::multimap<guid_t,std::map<uint32_t,Info>::iterator>::value_type(oid,p.first));	

		return nCookie;
	}
	catch (std::exception& e)
	{ 
		if (rot_cookie)
			ptrROT->Revoke(rot_cookie);

		OMEGA_THROW(e);
	}
	catch (...)
	{
		if (rot_cookie)
			ptrROT->Revoke(rot_cookie);

		throw;
	}
}

IObject* OOCore::ServiceManager::GetObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid)
{
	try
	{
		// Remove any flags we don't care about...
		flags &= ~(Activation::DontLaunch);

		OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		for (std::multimap<guid_t,std::map<uint32_t,Info>::iterator>::iterator i=m_mapServicesByOid.find(oid);i!=m_mapServicesByOid.end() && i->first==oid;++i)
		{
			if (i->second->second.m_flags & flags)
			{
				return i->second->second.m_ptrObject->QueryInterface(iid);
			}
		}

		// No, didn't find it
		return 0;
	}
	catch (std::exception& e)
	{ 
		OMEGA_THROW(e);
	}
}

void OOCore::ServiceManager::RevokeObject(uint32_t cookie)
{
	try
	{
		uint32_t rot_cookie = 0;
		
		{
			OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<uint32_t,Info>::iterator i = m_mapServicesByCookie.find(cookie);
			if (i == m_mapServicesByCookie.end())
				OMEGA_THROW(ENOENT);

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
		}

		if (rot_cookie)
		{
			// Revoke from ROT
			ObjectPtr<Activation::IRunningObjectTable> ptrROT;
			ptrROT.Attach(Activation::IRunningObjectTable::GetRunningObjectTable());

			// Change to use monikers...
			ptrROT->Revoke(rot_cookie);
		}
	}
	catch (std::exception& e)
	{ 
		OMEGA_THROW(e);
	}
}

// External declaration of our version of this entry point
void Omega_GetLibraryObject_Impl(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject);

IObject* OOCore::LoadLibraryObject(const string_t& dll_name, const guid_t& oid, Activation::Flags_t flags, const guid_t& iid)
{
	IObject* pObj = 0;

	ACE_DLL dll;
	if (dll_name != L"OOCore")
	{
		// Ensure we are using per-dll unloading
		ACE_DLL_Manager::instance()->unload_policy(ACE_DLL_UNLOAD_POLICY_PER_DLL);

		if (dll.open(dll_name.c_str()) != 0)
			LibraryNotFoundException::Throw(dll_name);

		typedef System::MetaInfo::IException_Safe* (OMEGA_CALL *pfnGetLibraryObject)(System::MetaInfo::marshal_info<const guid_t&>::safe_type::type oid, System::MetaInfo::marshal_info<Activation::Flags_t>::safe_type::type flags, System::MetaInfo::marshal_info<const guid_t&>::safe_type::type iid, System::MetaInfo::marshal_info<IObject*&>::safe_type::type pObject);
		pfnGetLibraryObject pfn = (pfnGetLibraryObject)dll.symbol(L"Omega_GetLibraryObject_Safe");
		if (pfn == 0)
			OOCORE_THROW_LASTERROR();

		System::MetaInfo::IException_Safe* GetLibraryObject_Exception = pfn(
			System::MetaInfo::marshal_info<const guid_t&>::safe_type::coerce(oid),
			System::MetaInfo::marshal_info<Activation::Flags_t>::safe_type::coerce(flags),
			System::MetaInfo::marshal_info<const guid_t&>::safe_type::coerce(iid),
			System::MetaInfo::marshal_info<IObject*&>::safe_type::coerce(pObj,iid));

		if (GetLibraryObject_Exception)
			System::MetaInfo::throw_correct_exception(GetLibraryObject_Exception);
	}
	else
	{
		// Its us!
		Omega_GetLibraryObject_Impl(oid,flags,iid,pObj);
	}

	return pObj;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(guid_t,Activation_NameToOid,1,((in),const string_t&,strObjectName))
{
	string_t strCurName = strObjectName;
	for (;;)
	{
		ObjectPtr<Registry::IRegistryKey> ptrOidKey(L"\\Objects\\" + strCurName);

		if (ptrOidKey->IsValue(L"CurrentVersion"))
		{
			strCurName = ptrOidKey->GetStringValue(L"CurrentVersion");
			continue;
		}

		return guid_t::FromString(ptrOidKey->GetStringValue(L"OID"));		
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Activation_GetRegisteredObject,4,((in),const Omega::guid_t&,oid,(in),Omega::Activation::Flags_t,flags,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject))
{
	pObject = 0;
	try
	{
		// Try ourselves first... this prevents anyone overloading standard behaviours!
		if (flags & Activation::InProcess)
		{
			Omega_GetLibraryObject_Impl(oid,flags,iid,pObject);
			if (pObject)
				return;
		}

		// Try the Service Manager
		pObject = OOCore::SERVICE_MANAGER::instance()->GetObject(oid,flags,iid);
		if (pObject)
			return;
		
		if ((flags & Activation::InProcess) && !(flags & Activation::DontLaunch))
		{
			// Use the registry
			ObjectPtr<Registry::IRegistryKey> ptrOidsKey(L"\\Objects\\OIDs");
			if (ptrOidsKey->IsSubKey(oid.ToString()))
			{
				ObjectPtr<Registry::IRegistryKey> ptrOidKey = ptrOidsKey.OpenSubKey(oid.ToString());
				if (ptrOidKey->IsValue(L"Library"))
				{
					pObject = OOCore::LoadLibraryObject(ptrOidKey->GetStringValue(L"Library"),oid,flags,iid);
					if (pObject)
						return;
				}
			}
		}

		if (flags & Activation::OutOfProcess)
		{
			ObjectPtr<Remoting::IInterProcessService> ptrIPC = OOCore::GetInterProcessService();
			ptrIPC->GetRegisteredObject(oid,flags,iid,pObject);
			if (pObject)
				return;
		}
	}
	catch (IException* pE)
	{
		ObjectPtr<IException> ptrE;
		ptrE.Attach(pE);

		OOCore::OidNotFoundException::Throw(oid,ptrE);
	}

	OOCore::OidNotFoundException::Throw(oid);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_CreateInstance,5,((in),const guid_t&,oid,(in),Activation::Flags_t,flags,(in),IObject*,pOuter,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
{
	ObjectPtr<Activation::IObjectFactory> ptrOF;
	IObject* pOF = Omega::Activation::GetRegisteredObject(oid,flags,OMEGA_UUIDOF(Activation::IObjectFactory));
	ptrOF.Attach(static_cast<Activation::IObjectFactory*>(pOF));
	ptrOF->CreateInstance(pOuter,iid,pObject);
}
