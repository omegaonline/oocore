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

#include "StdObjectManager.h"
#include "WireProxy.h"
#include "Channel.h"
#include "Exception.h"
#include "Activation.h"
#include "IPS.h"

#if defined(_WIN32)
#include <shlwapi.h>
#endif

#if !defined(DOXYGEN)

// Our library map
BEGIN_LIBRARY_OBJECT_MAP()
	OBJECT_MAP_ENTRY(OOCore::ProxyMarshalFactory)
	OBJECT_MAP_ENTRY(OOCore::ChannelMarshalFactory)
	OBJECT_MAP_ENTRY(OOCore::CDRMessageMarshalFactory)
	OBJECT_MAP_ENTRY(OOCore::StdObjectManager)
	OBJECT_MAP_ENTRY(OOCore::SystemExceptionMarshalFactoryImpl)
	OBJECT_MAP_ENTRY(OOCore::InternalExceptionMarshalFactoryImpl)
	OBJECT_MAP_ENTRY(OOCore::NoInterfaceExceptionMarshalFactoryImpl)
	OBJECT_MAP_ENTRY(OOCore::TimeoutExceptionMarshalFactoryImpl)
	OBJECT_MAP_ENTRY(OOCore::ChannelClosedExceptionMarshalFactoryImpl)
	OBJECT_MAP_ENTRY(OOCore::OidNotFoundExceptionMarshalFactoryImpl)
END_LIBRARY_OBJECT_MAP_NO_ENTRYPOINT()

#endif // DOXYGEN

using namespace Omega;
using namespace OTL;

OMEGA_DEFINE_OID(OOCore,OID_ServiceManager,"{60B09DE7-609E-4b82-BA35-270A9544BE29}");
OMEGA_DEFINE_OID(OOCore,OID_OidNotFoundExceptionMarshalFactory, "{0CA3037F-08C0-442a-B4EC-84A9156839CD}");

namespace
{
	class NoAggregationException :
			public ExceptionImpl<Activation::INoAggregationException>
	{
	public:
		any_t  m_oid;

		BEGIN_INTERFACE_MAP(NoAggregationException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<Activation::INoAggregationException>)
		END_INTERFACE_MAP()

	// Activation::INoAggregationException members
	public:
		any_t GetFailingOid()
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

	class DLLManagerImpl
	{
	public:
		DLLManagerImpl();
		~DLLManagerImpl();

		OOBase::SmartPtr<OOBase::DLL> load_dll(const string_t& name);
		void unload_unused();

	private:
		DLLManagerImpl(const DLLManagerImpl&);
		DLLManagerImpl& operator = (const DLLManagerImpl&);

		OOBase::Mutex                                          m_lock;
		OOBase::Table<string_t,OOBase::SmartPtr<OOBase::DLL> > m_dll_map;
	};
	typedef Threading::Singleton<DLLManagerImpl,Threading::InitialiseDestructor<OOCore::DLL> > DLLManager;

	DLLManagerImpl::DLLManagerImpl()
	{
	}

	DLLManagerImpl::~DLLManagerImpl()
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		// Clear out our map now, as the smart ptrs use m_lock
		m_dll_map.clear();
	}

	OOBase::SmartPtr<OOBase::DLL> DLLManagerImpl::load_dll(const string_t& name)
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		// See if we have it already
		OOBase::SmartPtr<OOBase::DLL> dll;
		if (m_dll_map.find(name,dll))
			return dll;

		// Try to unload any unused dlls
		unload_unused();

		dll = new (std::nothrow) OOBase::DLL();
		if (!dll)
			OMEGA_THROW_NOMEM();
	
		// Load the new DLL
		int err = dll->load(name.c_nstr());
		if (err != 0)
			throw ISystemException::Create(err,OMEGA_CREATE_INTERNAL((L"Loading library: " + name).c_nstr()));
				
		// Add to the map
		err = m_dll_map.insert(name,dll);
		if (err != 0)
			OMEGA_THROW(err);

		return dll;
	}

	void DLLManagerImpl::unload_unused()
	{
		typedef System::Internal::SafeShim* (OMEGA_CALL *pfnCanUnloadLibrary)(System::Internal::marshal_info<bool_t&>::safe_type::type result);

		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		for (size_t i=0; i<m_dll_map.size();)
		{
			string_t name = *m_dll_map.key_at(i);
			OOBase::SmartPtr<OOBase::DLL> dll = *m_dll_map.at(i);

			guard.release();

			bool_t erase = false;
			try
			{
				pfnCanUnloadLibrary pfn = (pfnCanUnloadLibrary)(dll->symbol("Omega_CanUnloadLibrary_Safe"));
				if (pfn)
				{
					System::Internal::SafeShim* CanUnloadLibrary_Exception = pfn(System::Internal::marshal_info<bool_t&>::safe_type::coerce(erase));

					// Ignore exceptions
					if (CanUnloadLibrary_Exception)
						System::Internal::release_safe(CanUnloadLibrary_Exception);
				}
			}
			catch (IException* pE)
			{
				// Ignore exceptions
				pE->Release();
			}

			guard.acquire();

			if (erase)
				m_dll_map.erase(name);
			else
				++i;
		}
	}

	void LibraryNotFoundException::Throw(const string_t& strName, IException* pE)
	{
		ObjectImpl<LibraryNotFoundException>* pRE = ObjectImpl<LibraryNotFoundException>::CreateInstance();
		pRE->m_ptrCause.Attach(pE);
		pRE->m_strDesc = L"Dynamic library '{0}' not found or malformed." % strName;
		pRE->m_dll_name = strName;
		throw static_cast<ILibraryNotFoundException*>(pRE);
	}

	IObject* LoadLibraryObject(const string_t& dll_name, const guid_t& oid, const guid_t& iid)
	{
		typedef System::Internal::SafeShim* (OMEGA_CALL *pfnGetLibraryObject)(System::Internal::marshal_info<const guid_t&>::safe_type::type oid, System::Internal::marshal_info<const guid_t&>::safe_type::type iid, System::Internal::marshal_info<IObject*&>::safe_type::type pObject);
		pfnGetLibraryObject pfn = 0;
		OOBase::SmartPtr<OOBase::DLL> dll;

		try
		{
			dll = DLLManager::instance()->load_dll(dll_name);
			pfn = (pfnGetLibraryObject)dll->symbol("Omega_GetLibraryObject_Safe");
		}
		catch (IException* pE)
		{
			LibraryNotFoundException::Throw(dll_name,pE);
		}

		IObject* pObj = 0;
		const System::Internal::SafeShim* GetLibraryObject_Exception = pfn(
					System::Internal::marshal_info<const guid_t&>::safe_type::coerce(oid),
					System::Internal::marshal_info<const guid_t&>::safe_type::coerce(iid),
					System::Internal::marshal_info<IObject*&>::safe_type::coerce(pObj,iid));

		if (GetLibraryObject_Exception)
			System::Internal::throw_correct_exception(GetLibraryObject_Exception);

		return pObj;
	}

	bool IsRelativePath(const string_t& strPath)
	{
#if defined(_WIN32)
		return (PathIsRelativeW(strPath.c_wstr()) != FALSE);
#else
		return (strPath[0] != L'/');
#endif
	}

	IObject* LoadObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid)
	{
		// Try to load a library, if allowed
		if (flags & Activation::InProcess)
		{
			// Use the registry
			ObjectPtr<Registry::IKey> ptrOidKey(L"/Local User/Objects/OIDs/" + oid.ToString());
			if (ptrOidKey && ptrOidKey->IsValue(L"Library"))
			{
				string_t strLib = ptrOidKey->GetValue(L"Library").cast<string_t>();
				if (IsRelativePath(strLib))
				{
					string_t strErr(L"Relative path \"{0}\" in object library '{1}' activation registry value." % strLib % oid);
					OMEGA_THROW(strErr.c_nstr());
				}

				void* ISSUE_8; // Surrogates here?!?

				IObject* pObject = LoadLibraryObject(strLib,oid,iid);
				if (pObject)
					return pObject;
			}
		}

		// Try out-of-process...
		if (flags & Activation::OutOfProcess)
		{
			// Ask the IPS to run it...
			ObjectPtr<OOCore::IInterProcessService> ptrIPS = OOCore::GetInterProcessService();
			if (ptrIPS)
			{
				IObject* pObject = 0;
				ptrIPS->LaunchObjectApp(oid,iid,pObject);
				return pObject;
			}
		}

		return 0;
	}

	IObject* GetLocalInstance(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid)
	{
		IObject* pObject = 0;

		// Try ourselves first... this prevents anyone overloading standard behaviours!
		if (flags & Activation::InProcess)
		{
			if (oid == OOCore::OID_ServiceManager)
			{
				pObject = SingletonObjectImpl<OOCore::ServiceManager>::CreateInstancePtr()->QueryInterface(iid);
				if (!pObject)
					throw INoInterfaceException::Create(iid);
				return pObject;
			}

			pObject = OTL::Module::OMEGA_PRIVATE_FN_CALL(GetModule)()->GetLibraryObject(oid,iid);
			if (pObject)
				return pObject;
		}

		// Build RegisterFlags
		Activation::RegisterFlags_t reg_mask = 0;
		if (flags & Activation::InProcess)
			reg_mask |= Activation::ProcessLocal;

		if (flags & Activation::OutOfProcess)
			reg_mask |= Activation::UserLocal | Activation::MachineLocal;

		// Surrogates must not be ProcessLocal
		if (flags & (Activation::Surrogate | Activation::PrivateSurrogate))
		{
			reg_mask |= Activation::UserLocal;
			reg_mask &= ~Activation::ProcessLocal;
		}

		// Sandbox must be not be UserLocal
		if (flags & (Activation::Sandbox | Activation::VM))
			reg_mask &= ~(Activation::UserLocal | Activation::ProcessLocal);

		// Remote activation
		if (flags & Activation::RemoteActivation)
			reg_mask |= Activation::Anywhere;

		// See if we have it registered ion the ROT
		ObjectPtr<Activation::IRunningObjectTable> ptrROT;
		ptrROT.Attach(Activation::IRunningObjectTable::GetRunningObjectTable());

		ptrROT->GetObject(oid,reg_mask,iid,pObject);
		if (pObject)
			return pObject;

		void* ISSUE_10; // Allow injection of callback

		// See if we are allowed to load...
		if (!(flags & Activation::DontLaunch))
		{
			pObject = LoadObject(oid,flags,iid);
			if (pObject)
				return pObject;
		}

		return 0;
	}

	guid_t NameToOid(const string_t& strObjectName)
	{
		string_t strCurName = strObjectName;
		for (;;)
		{
			try
			{
				ObjectPtr<Registry::IKey> ptrOidKey(L"/Local User/Objects/" + strCurName);
				if (ptrOidKey->IsValue(L"CurrentVersion"))
				{
					strCurName = ptrOidKey->GetValue(L"CurrentVersion").cast<string_t>();
					continue;
				}

				return ptrOidKey->GetValue(L"OID").cast<guid_t>();
			}
			catch (IException* pE)
			{
				OOCore::OidNotFoundException::Throw(strCurName,pE);
			}
		}
	}
}

void OOCore::OidNotFoundException::Throw(const any_t& oid, IException* pE)
{
	ObjectImpl<OidNotFoundException>* pNew = ObjectImpl<OidNotFoundException>::CreateInstance();
	pNew->m_strDesc = L"The identified object could not be found: {0}" % oid;
	pNew->m_ptrCause.Attach(pE);
	pNew->m_oid = oid;
	throw static_cast<IOidNotFoundException*>(pNew);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Activation::INoAggregationException*,OOCore_Activation_INoAggregationException_Create,1,((in),const any_t&,oid))
{
	ObjectImpl<NoAggregationException>* pNew = ObjectImpl<NoAggregationException>::CreateInstance();
	pNew->m_strDesc = L"Object does not support aggregation.";
	pNew->m_oid = oid;
	return pNew;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Activation::IOidNotFoundException*,OOCore_Activation_IOidNotFoundException_Create,1,((in),const any_t&,oid))
{
	ObjectImpl<OOCore::OidNotFoundException>* pNew = ObjectImpl<OOCore::OidNotFoundException>::CreateInstance();
	pNew->m_strDesc = L"The identified object could not be found: {0}" % oid;
	pNew->m_oid = oid;
	return pNew;
}

IObject* OOCore::GetInstance(const any_t& oid, Activation::Flags_t flags, const guid_t& iid)
{
	try
	{
		// First try to determine the protocol...
		guid_t oid_guid;
		if (oid.Coerce(oid_guid) == any_t::castValid)
		{
			IObject* pObject = GetLocalInstance(oid_guid,flags,iid);
			if (!pObject)
				OidNotFoundException::Throw(oid);
			return pObject;
		}

		string_t strObject = oid.cast<string_t>();
		string_t strEndpoint;
		size_t pos = strObject.Find(L'@');
		if (pos != string_t::npos)
		{
			strEndpoint = strObject.Mid(pos+1).ToLower();
			strObject = strObject.Left(pos);

			if (strEndpoint == L"local")
				strEndpoint.Clear();
		}

		if (strEndpoint.IsEmpty())
		{
			// Do a quick registry lookup
			if (!guid_t::FromString(strObject,oid_guid))
				oid_guid = NameToOid(strObject);

			IObject* pObject = GetLocalInstance(oid_guid,flags,iid);
			if (!pObject)
				OidNotFoundException::Throw(oid);
			return pObject;
		}

		// Get IPS
		ObjectPtr<OOCore::IInterProcessService> ptrIPS = OOCore::GetInterProcessService();
		if (!ptrIPS)
			throw IInternalException::Create("Omega::Initialize not called","OOCore");

		// Open a remote channel
		ObjectPtr<Remoting::IChannel> ptrChannel;
		ptrChannel.Attach(ptrIPS->OpenRemoteChannel(strEndpoint));

		// Get the ObjectManager
		ObjectPtr<Remoting::IObjectManager> ptrOM = ptrChannel.GetManager<Remoting::IObjectManager>();

		// Get the remote instance
		IObject* pObject = 0;
		ptrOM->GetRemoteInstance(strObject,flags,iid,pObject);
		if (!pObject)
			OidNotFoundException::Throw(oid);

		return pObject;
	}
	catch (Activation::IOidNotFoundException* pE)
	{
		pE->Rethrow();
	}
	catch (INoInterfaceException* pE)
	{
		pE->Rethrow();
	}
	catch (IException* pE)
	{
		OidNotFoundException::Throw(oid,pE);
	}

	return 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Activation::IObjectFactory*,OOCore_GetObjectFactory,2,((in),const any_t&,oid,(in),Activation::Flags_t,flags))
{
	return static_cast<Activation::IObjectFactory*>(OOCore::GetInstance(oid,flags,OMEGA_GUIDOF(Activation::IObjectFactory)));
}
