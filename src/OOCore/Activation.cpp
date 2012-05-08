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

#if defined(_WIN32)
#include <shlwapi.h>
#endif

using namespace Omega;
using namespace OTL;

namespace
{
	class DLLManagerImpl
	{
	public:
		DLLManagerImpl()
		{ }

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

	IObject* LoadLibraryObject(const string_t& dll_name, const guid_t& oid, const guid_t& iid)
	{
		typedef System::Internal::SafeShim* (OMEGA_CALL *pfnGetLibraryObject)(System::Internal::marshal_info<const guid_t&>::safe_type::type oid, System::Internal::marshal_info<const guid_t&>::safe_type::type iid, System::Internal::marshal_info<IObject*&>::safe_type::type pObject);

		OOBase::SmartPtr<OOBase::DLL> dll = DLLManager::instance()->load_dll(dll_name);
		pfnGetLibraryObject pfn = (pfnGetLibraryObject)dll->symbol("Omega_GetLibraryObject_Safe");
		if (!pfn)
			throw INotFoundException::Create(OOCore::get_text("The library {0} is missing the Omega_GetLibraryObject_Safe entrypoint") % dll_name);

		IObject* pObj = NULL;
		const System::Internal::SafeShim* GetLibraryObject_Exception = pfn(
					System::Internal::marshal_info<const guid_t&>::safe_type::coerce(oid),
					System::Internal::marshal_info<const guid_t&>::safe_type::coerce(iid),
					System::Internal::marshal_info<IObject*&>::safe_type::coerce(pObj,iid));

		System::Internal::throw_correct_exception(GetLibraryObject_Exception);
		return pObj;
	}

#if defined(_WIN32)
	OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> to_wchar_t(const Omega::string_t& str)
	{
		OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> wsz;
		int len = MultiByteToWideChar(CP_UTF8,0,str.c_str(),-1,NULL,0);
		if (len == 0)
		{
			DWORD dwErr = GetLastError();
			if (dwErr != ERROR_INSUFFICIENT_BUFFER)
				OMEGA_THROW(dwErr);
		}

		wsz = static_cast<wchar_t*>(OOBase::LocalAllocator::allocate((len+1) * sizeof(wchar_t)));
		if (!wsz)
			OMEGA_THROW(ERROR_OUTOFMEMORY);

		MultiByteToWideChar(CP_UTF8,0,str.c_str(),-1,wsz,len);
		wsz[len] = L'\0';
		return wsz;
	}

	string_t from_wchar_t(const wchar_t* wstr)
	{
		string_t ret;
		char szBuf[1024] = {0};
		int len = WideCharToMultiByte(CP_UTF8,0,wstr,-1,szBuf,sizeof(szBuf)-1,NULL,NULL);
		if (len != 0)
			ret = string_t(szBuf,len);
		else
		{
			DWORD dwErr = GetLastError();
			if (dwErr != ERROR_INSUFFICIENT_BUFFER)
				OMEGA_THROW(dwErr);
		
			len = WideCharToMultiByte(CP_UTF8,0,wstr,-1,NULL,0,NULL,NULL);
			char* sz = static_cast<char*>(OOBase::LocalAllocator::allocate(len + 1));
			if (!sz)
				OMEGA_THROW(ERROR_OUTOFMEMORY);

			len = WideCharToMultiByte(CP_UTF8,0,wstr,-1,sz,len,NULL,NULL);
			string_t(szBuf,len);
			OOBase::LocalAllocator::free(sz);
		}
		
		return ret;
	}
#endif

	bool IsRelativePath(const string_t& strPath)
	{
#if defined(_WIN32)
		return (PathIsRelativeW(to_wchar_t(strPath)) != FALSE);
#else
		return (strPath[0] != '/');
#endif
	}

	ObjectPtr<Registry::IKey> GetObjectsKey(const string_t& strSubKey)
	{
		ObjectPtr<Registry::IKey> ptrObjects = ObjectPtr<Registry::IOverlayKeyFactory>(Registry::OID_OverlayKeyFactory)->Overlay(string_t::constant("Local User/Objects"),string_t::constant("All Users/Objects"));
		return ptrObjects->OpenKey(strSubKey);
	}

	IObject* RunSurrogateObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid)
	{
		string_t strOid = string_t::constant("Omega.Surrogate");
		if (flags & Activation::OwnSurrogate)
			strOid = string_t::constant("Omega.SingleSurrogate");

		IObject* pObject = NULL;
		ObjectPtr<OOCore::ISurrogate> ptrSurrogate(strOid,flags);
		if (ptrSurrogate)
			ptrSurrogate->CreateInstance(oid,iid,flags,pObject);

		return pObject;
	}

	IObject* LoadObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid)
	{
		Activation::Flags_t sub_type = (flags & 0xF);

		// Check for Library key in the registry first
		string_t strLib;
		ObjectPtr<Registry::IKey> ptrOidKey = GetObjectsKey("OIDs/" + oid.ToString());
		if (ptrOidKey->IsValue(Omega::string_t::constant("Library")))
		{
			strLib = ptrOidKey->GetValue(Omega::string_t::constant("Library")).cast<string_t>();
			if (strLib.IsEmpty() || IsRelativePath(strLib))
				throw IAccessDeniedException::Create(OOCore::get_text("Relative path \"{0}\" in library registry value.") % strLib);
		}
		
		if (!strLib.IsEmpty())
		{
			// Try to load a library, if allowed
			if (sub_type == Activation::Default || sub_type == Activation::Library)
			{
				// Load the library locally
				IObject* pObject = LoadLibraryObject(strLib,oid,iid);
				if (pObject)
					return pObject;
			}

			if (sub_type != Activation::Library)
			{
				// Run a surrogate
				IObject* pObject = RunSurrogateObject(oid,flags,iid);
				if (pObject)
					return pObject;
			}
		}
		else if (sub_type != Activation::Library)
		{
			// Ask the IPS to run it...
			ObjectPtr<OOCore::IInterProcessService> ptrIPS = OOCore::GetInterProcessService();
			if (ptrIPS)
			{
				uint32_t envc = 0;
				OOBase::SmartPtr<string_t,OOBase::ArrayDeleteDestructor<string_t> > envp;

#if defined(_WIN32)
				const wchar_t* env = GetEnvironmentStringsW();
				for (const wchar_t* e=env;e != NULL && *e != L'\0';++envc)
					e += wcslen(e)+1;

				if (envc)
				{
					envp = new (OOCore::throwing) string_t[envc];

					size_t i = 0;
					for (const wchar_t* e=env;e != NULL && *e != L'\0';++i)
					{
						envp[i] = from_wchar_t(e);
						e += wcslen(e)+1;
					}
				}
#elif defined(HAVE_UNISTD_H)
				for (char** e=environ;*e != NULL;++e)
					++envc;

				if (envc)
				{
					envp = new (OOCore::throwing) string_t[envc];

					size_t i = 0;
					for (char** e=environ;*e != NULL;++e)
						envp[i++] = string_t(*e);
				}
#else
#error Fix me!
#endif
				IObject* pObject = NULL;
				ptrIPS->LaunchObjectApp(oid,iid,flags,envc,envp,pObject);
				return pObject;
			}
		}

		return NULL;
	}

	IObject* GetLocalInstance(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid)
	{
		// Check for remote activation
		bool remote_activation = false;
		if (flags & Activation::RemoteActivation)
			remote_activation = true;
			
		// See if we have it registered in the ROT
		ObjectPtr<Activation::IRunningObjectTable> ptrROT = SingletonObjectImpl<OOCore::LocalROT>::CreateInstance();

		IObject* pObject = NULL;
		ptrROT->GetObject(oid,iid,pObject,remote_activation);
		if (!pObject)
		{
			// See if we are allowed to load...
			if (flags & Activation::DontLaunch)
				throw INotFoundException::Create(OOCore::get_text("A library or running instance of object {0} could not be found") % oid);

			pObject = LoadObject(oid,flags,iid);
		}

		return pObject;
	}

	guid_t NameToOid(const string_t& strObjectName)
	{
		string_t strCurName = strObjectName;
		ObjectPtr<Registry::IKey> ptrOidKey = GetObjectsKey(strCurName);

		while (ptrOidKey->IsValue(Omega::string_t::constant("CurrentVersion")))
		{
			strCurName = ptrOidKey->GetValue(Omega::string_t::constant("CurrentVersion")).cast<string_t>();
			ptrOidKey = GetObjectsKey(strCurName);
		}

		return ptrOidKey->GetValue(Omega::string_t::constant("OID")).cast<guid_t>();
	}
}

template class Threading::Singleton<DLLManagerImpl,Threading::InitialiseDestructor<OOCore::DLL> >;

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

	dll = new (OOCore::throwing) OOBase::DLL();

	// Load the new DLL
	int err = dll->load(name.c_str());
	if (err != 0)
	{
		ObjectPtr<IException> ptrE = ISystemException::Create(err);
		throw INotFoundException::Create(OOCore::get_text("Failed to load library {0}") % name,ptrE);
	}

	// Add to the map
	if ((err = m_dll_map.insert(name,dll)) != 0)
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
				{
					System::Internal::release_safe(CanUnloadLibrary_Exception);
					erase = false;
				}
			}
		}
		catch (IException* pE)
		{
			// Ignore exceptions
			pE->Release();
			erase = false;
		}

		guard.acquire();

		if (erase)
			m_dll_map.remove(name);
		else
			++i;
	}
}

IObject* OOCore::GetInstance(const any_t& oid, Activation::Flags_t flags, const guid_t& iid)
{
	try
	{
		// First try to determine the protocol...
		guid_t oid_guid;
		if (oid.Coerce(oid_guid) == any_t::castValid)
			return GetLocalInstance(oid_guid,flags,iid);

		string_t strObject = oid.cast<string_t>();
		string_t strEndpoint;
		size_t pos = strObject.Find('@');
		if (pos != string_t::npos)
		{
			strEndpoint = strObject.Mid(pos+1);
			strObject = strObject.Left(pos);

			if (strEndpoint == "local")
				strEndpoint.Clear();
		}

		if (strEndpoint.IsEmpty())
		{
			// Do a quick registry lookup
			if (!guid_t::FromString(strObject,oid_guid))
				oid_guid = NameToOid(strObject);

			return GetLocalInstance(oid_guid,flags,iid);
		}

		// Open a remote channel
		ObjectPtr<Remoting::IChannel> ptrChannel = OOCore::GetInterProcessService()->OpenRemoteChannel(strEndpoint);

		// Get the ObjectManager
		IObject* pObject = NULL;
		ptrChannel->GetManager(OMEGA_GUIDOF(Remoting::IObjectManager),pObject);
		ObjectPtr<Remoting::IObjectManager> ptrOM = static_cast<Remoting::IObjectManager*>(pObject);

		// Get the remote instance
		pObject = NULL;
		ptrOM->GetRemoteInstance(strObject,flags,iid,pObject);
		return pObject;
	}
	catch (IException* pE)
	{
		ObjectPtr<IException> ptrE = pE;
		throw INotFoundException::Create(OOCore::get_text("The requested object {0} could not be created") % oid,pE);
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Activation::IObjectFactory*,OOCore_GetObjectFactory,2,((in),const any_t&,oid,(in),Activation::Flags_t,flags))
{
	return static_cast<Activation::IObjectFactory*>(OOCore::GetInstance(oid,flags,OMEGA_GUIDOF(Activation::IObjectFactory)));
}

// {EAAC4365-9B65-4C3C-94C2-CC8CC3E64D74}
OMEGA_DEFINE_OID(Registry,OID_Registry,"{EAAC4365-9B65-4C3C-94C2-CC8CC3E64D74}");

// {7A351233-8363-BA15-B443-31DD1C8FC587}
OMEGA_DEFINE_OID(Registry,OID_OverlayKeyFactory,"{7A351233-8363-BA15-B443-31DD1C8FC587}");

void OOCore::RegistryFactory::CreateInstance(const guid_t& iid, IObject*& pObject)
{
	ObjectPtr<Registry::IKey> ptrKey = OOCore::GetInterProcessService()->GetRegistry();
	if (ptrKey)
		pObject = ptrKey->QueryInterface(iid);
}
