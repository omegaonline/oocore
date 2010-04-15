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
#include "ApartmentImpl.h"
#include "WireProxy.h"
#include "Channel.h"
#include "Exception.h"
#include "Activation.h"
#include "IPS.h"

#if !defined(DOXYGEN)

// Our library map
BEGIN_LIBRARY_OBJECT_MAP()
	OBJECT_MAP_ENTRY(OOCore::StdObjectManager,0)
	OBJECT_MAP_ENTRY(OOCore::ApartmentImpl,0)
	OBJECT_MAP_ENTRY(OOCore::ProxyMarshalFactory,0)
	OBJECT_MAP_ENTRY(OOCore::ChannelMarshalFactory,0)
	OBJECT_MAP_ENTRY(OOCore::CDRMessageMarshalFactory,0)
	OBJECT_MAP_ENTRY(OOCore::SystemExceptionMarshalFactoryImpl,0)
	OBJECT_MAP_ENTRY(OOCore::NoInterfaceExceptionMarshalFactoryImpl,0)
	OBJECT_MAP_ENTRY(OOCore::TimeoutExceptionMarshalFactoryImpl,0)
	OBJECT_MAP_ENTRY(OOCore::ChannelClosedExceptionMarshalFactoryImpl,0)
END_LIBRARY_OBJECT_MAP_NO_REGISTRATION()

#endif // DOXYGEN

using namespace Omega;
using namespace OTL;

namespace
{
	class OidNotFoundException :
		public ExceptionImpl<Omega::Activation::IOidNotFoundException>
	{
	public:
		guid_t m_oid;

		static void Throw(const guid_t& Oid, const string_t& strFn, IException* pE = 0);

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
		static void Throw(const string_t& strName, const string_t& strFn, IException* pE = 0);

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

		OOBase::Mutex                                     m_lock;
		std::map<string_t,OOBase::SmartPtr<OOBase::DLL> > m_dll_map;
	};
	typedef Threading::Singleton<DLLManagerImpl,Threading::InitialiseDestructor<OOCore::DLL> > DLLManager;

	DLLManagerImpl::DLLManagerImpl()
	{
	}

	DLLManagerImpl::~DLLManagerImpl()
	{
		try
		{
			OOBase::Guard<OOBase::Mutex> guard(m_lock);

			// Clear out our map now, as the smart ptrs use m_lock
			m_dll_map.clear();
		}
		catch (...)
		{ }
	}

	OOBase::SmartPtr<OOBase::DLL> DLLManagerImpl::load_dll(const string_t& name)
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		// See if we have it already
		try
		{
			std::map<string_t,OOBase::SmartPtr<OOBase::DLL> >::iterator i=m_dll_map.find(name);
			if (i != m_dll_map.end())
				return i->second;
		}
		catch (std::exception& e)
		{
			OMEGA_THROW(e);
		}

		// Try to unload any unused dlls
		unload_unused();

		OOBase::SmartPtr<OOBase::DLL> dll;
		OMEGA_NEW(dll,OOBase::DLL());

		// Load the new DLL
		int err = dll->load(name.ToUTF8().c_str());
		if (err != 0)
			OMEGA_THROW(err);

		// Add to the map
		try
		{
			m_dll_map.insert(std::map<string_t,OOBase::SmartPtr<OOBase::DLL> >::value_type(name,dll));
		}
		catch (std::exception& e)
		{
			OMEGA_THROW(e);
		}

		return dll;
	}

	void DLLManagerImpl::unload_unused()
	{
		typedef System::Internal::SafeShim* (OMEGA_CALL *pfnCanUnloadLibrary)(System::Internal::marshal_info<bool_t&>::safe_type::type result);
			
		try
		{
			OOBase::Guard<OOBase::Mutex> guard(m_lock);

			for (std::map<string_t,OOBase::SmartPtr<OOBase::DLL> >::iterator i=m_dll_map.begin();i!=m_dll_map.end();)
			{
				bool_t erase = false;
				try
				{
					pfnCanUnloadLibrary pfn = (pfnCanUnloadLibrary)(i->second->symbol("Omega_CanUnloadLibrary_Safe"));
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

				if (erase)
					m_dll_map.erase(i++);
				else
					++i;
			}
		}
		catch (std::exception& e)
		{
			OMEGA_THROW(e);
		}
		
	}

	void OidNotFoundException::Throw(const guid_t& oid, const string_t& strFn, IException* pE)
	{
		ObjectImpl<OidNotFoundException>* pNew = ObjectImpl<OidNotFoundException>::CreateInstance();
		pNew->m_strDesc = L"The identified object could not be found: {0}";
		pNew->m_strDesc %= oid;
		pNew->m_strSource = strFn;
		pNew->m_ptrCause = pE;
		pNew->m_oid = oid;
		throw static_cast<IOidNotFoundException*>(pNew);
	}

	void LibraryNotFoundException::Throw(const string_t& strName, const string_t& strFn, IException* pE)
	{
		ObjectImpl<LibraryNotFoundException>* pRE = ObjectImpl<LibraryNotFoundException>::CreateInstance();
		pRE->m_ptrCause = pE;
		pRE->m_strDesc = L"Dynamic library '{0}' not found or malformed.";
		pRE->m_strDesc %= strName;
		pRE->m_strSource = strFn;
		pRE->m_dll_name = strName;
		throw static_cast<ILibraryNotFoundException*>(pRE);
	}
}

IObject* OOCore::ServiceManager::LoadLibraryObject(const string_t& dll_name, const guid_t& oid, Activation::Flags_t flags, const guid_t& iid)
{
	typedef System::Internal::SafeShim* (OMEGA_CALL *pfnGetLibraryObject)(System::Internal::marshal_info<const guid_t&>::safe_type::type oid, System::Internal::marshal_info<Activation::Flags_t>::safe_type::type flags, System::Internal::marshal_info<const guid_t&>::safe_type::type iid, System::Internal::marshal_info<IObject*&>::safe_type::type pObject);
	pfnGetLibraryObject pfn = 0;
	OOBase::SmartPtr<OOBase::DLL> dll;
	
	try
	{
		dll = DLLManager::instance()->load_dll(dll_name);
		pfn = (pfnGetLibraryObject)dll->symbol("Omega_GetLibraryObject_Safe");
	}
	catch (IException* pE)
	{
		LibraryNotFoundException::Throw(dll_name,L"Omega::Activation::GetRegisteredObject",pE);
	}

	IObject* pObj = 0;
	const System::Internal::SafeShim* GetLibraryObject_Exception = pfn(
		System::Internal::marshal_info<const guid_t&>::safe_type::coerce(oid)
		,flags,
		System::Internal::marshal_info<const guid_t&>::safe_type::coerce(iid),
		System::Internal::marshal_info<IObject*&>::safe_type::coerce(pObj,iid));

	if (GetLibraryObject_Exception)
		System::Internal::throw_correct_exception(GetLibraryObject_Exception);
	
	return pObj;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::Activation::INoAggregationException*,OOCore_Activation_INoAggregationException_Create,1,((in),const guid_t&,oid))
{
	ObjectImpl<NoAggregationException>* pNew = ObjectImpl<NoAggregationException>::CreateInstance();
	pNew->m_strDesc = L"Object does not support aggregation.";
	pNew->m_oid = oid;
	return pNew;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::uint32_t,OOCore_Activation_RegisterObject,4,((in),const Omega::guid_t&,oid,(in),Omega::IObject*,pObject,(in),Omega::Activation::Flags_t,flags,(in),Omega::Activation::RegisterFlags_t,reg_flags))
{
	if (!pObject)
		OMEGA_THROW(L"Do not register NULL object pointers");

	uint32_t ret = OOCore::SERVICE_MANAGER::instance()->RegisterObject(oid,pObject,flags,reg_flags);

	// This forces the detection, so cleanup succeeds
	OOCore::HostedByOOServer();

	return ret;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_Activation_RevokeObject,1,((in),Omega::uint32_t,cookie))
{
	OOCore::SERVICE_MANAGER::instance()->RevokeObject(cookie);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(guid_t,OOCore_Activation_NameToOid,1,((in),const string_t&,strObjectName))
{
	string_t strCurName = strObjectName;
	for (;;)
	{
		ObjectPtr<Registry::IKey> ptrOidKey(L"\\Local User\\Objects\\" + strCurName);
		if (ptrOidKey->IsValue(L"CurrentVersion"))
		{
			strCurName = ptrOidKey->GetStringValue(L"CurrentVersion");
			continue;
		}

		return guid_t::FromString(ptrOidKey->GetStringValue(L"OID"));
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_Activation_GetRegisteredObject,4,((in),const Omega::guid_t&,oid,(in),Omega::Activation::Flags_t,flags,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject))
{
	pObject = 0;
	try
	{
		// Try ourselves first... this prevents anyone overloading standard behaviours!
		if (flags & Activation::InProcess)
		{
			void* TODO; // Allow injection of callback

			pObject = OTL::Module::OMEGA_PRIVATE_FN_CALL(GetModule)()->GetLibraryObject(oid,flags,iid);
			if (pObject)
				return;
		}

		// Try the Service Manager
		pObject = OOCore::SERVICE_MANAGER::instance()->GetObject(oid,flags,iid);
		if (pObject)
			return;

		// Try to load a library, if allowed
		if ((flags & Activation::InProcess) && !(flags & Activation::DontLaunch))
		{
			// Use the registry
			ObjectPtr<Registry::IKey> ptrOidKey(L"\\Local User\\Objects\\OIDs\\" + oid.ToString());
			if (ptrOidKey && ptrOidKey->IsValue(L"Library"))
			{
				void* TICKET_89; // Surrogates here?!?

				pObject = OOCore::ServiceManager::LoadLibraryObject(ptrOidKey->GetStringValue(L"Library"),oid,flags,iid);
				if (pObject)
					return;
			}
		}

		// Try out-of-process...
		if (flags & Activation::OutOfProcess)
		{
			// Try RunningObjectTable first
			ObjectPtr<Activation::IRunningObjectTable> ptrROT;
			ptrROT.Attach(Activation::IRunningObjectTable::GetRunningObjectTable());
			
			// Change this to use monikers
			void* TICKET_90;

			ObjectPtr<IObject> ptrObject;
			ptrObject.Attach(ptrROT->GetObject(oid));
			if (ptrObject)
			{
				pObject = ptrObject->QueryInterface(iid);
				if (!pObject)
					throw INoInterfaceException::Create(iid);
				return;
			}

			if (!(flags & Activation::DontLaunch))
			{
				// Ask the IPS to run it...
				OOCore::GetInterProcessService()->LaunchObjectApp(oid,iid,pObject);
				if (pObject)
					return;
			}
		}
	}
	catch (IException* pE)
	{
		ObjectPtr<IException> ptrE;
		ptrE.Attach(pE);

		OidNotFoundException::Throw(oid,L"Omega::Activation::GetRegisteredObject",ptrE);
	}

	OidNotFoundException::Throw(oid,L"Omega::Activation::GetRegisteredObject");
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::Activation::IObjectFactory*,OOCore_GetObjectFactory,2,((in),const Omega::string_t&,strURI,(in),Omega::Activation::Flags_t,flags))
{
	// First try to determine the protocol...
	string_t strObject = strURI;
	string_t strEndpoint;
	size_t pos = strURI.Find(L'@');
	if (pos != string_t::npos)
	{
		strObject = strURI.Left(pos);
		strEndpoint = strURI.Mid(pos+1).ToLower();
		if (strEndpoint == L"local")
			strEndpoint.Clear();
	}

	if (strEndpoint.IsEmpty())
	{
		// Do a quick registry lookup
		guid_t oid;
		if (!guid_t::FromString(strObject,oid))
			oid = Omega::Activation::NameToOid(strObject);

		return static_cast<Activation::IObjectFactory*>(Activation::GetRegisteredObject(oid,flags,OMEGA_GUIDOF(Activation::IObjectFactory)));
	}
	else
	{
		// Get IPS
		ObjectPtr<OOCore::IInterProcessService> ptrIPS = OOCore::GetInterProcessService();

		// Open a remote channel
		ObjectPtr<Remoting::IChannel> ptrChannel;
		ptrChannel.Attach(ptrIPS->OpenRemoteChannel(strEndpoint));

		// Get the ObjectManager
		ObjectPtr<Remoting::IObjectManager> ptrOM = ptrChannel.GetManager<Remoting::IObjectManager>();

		// Get the remote instance
		IObject* pOF = 0;
		ptrOM->GetRemoteInstance(strObject,flags,OMEGA_GUIDOF(Activation::IObjectFactory),pOF);
		
		if (!pOF)
		{
			void* TODO; // Throw some kind of exception
			OMEGA_THROW(L"Failed to create object factory");
		}

		return static_cast<Activation::IObjectFactory*>(pOF);
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Activation::IRunningObjectTable*,OOCore_Activation_GetRunningObjectTable,0,())
{
	return OOCore::GetInterProcessService()->GetRunningObjectTable();
}
