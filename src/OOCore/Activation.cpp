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

		OOBase::SmartPtr<OOBase::DLL> load_dll(const string_t& name, bool allow_null);
		void unload_unused();
		bool can_unload();

	private:
		DLLManagerImpl(const DLLManagerImpl&);
		DLLManagerImpl& operator = (const DLLManagerImpl&);

		OOBase::Mutex                                          m_lock;
		OOBase::Table<string_t,OOBase::SmartPtr<OOBase::DLL> > m_dll_map;
	};
	typedef Threading::Singleton<DLLManagerImpl,Threading::InitialiseDestructor<OOCore::DLL> > DLLManager;

	typedef System::Internal::SafeShim* (OMEGA_CALL *pfnCanUnloadLibrary)(System::Internal::marshal_info<bool_t&>::safe_type::type result);

	IObject* LoadLibraryObject(const string_t& dll_name, const guid_t& oid, const guid_t& iid, bool allow_null)
	{
		typedef System::Internal::SafeShim* (OMEGA_CALL *pfnGetLibraryObject)(System::Internal::marshal_info<const guid_t&>::safe_type::type oid, System::Internal::marshal_info<const guid_t&>::safe_type::type iid, System::Internal::marshal_info<IObject*&>::safe_type::type pObject);

		OOBase::SmartPtr<OOBase::DLL> dll = DLLManager::instance()->load_dll(dll_name,allow_null);
		if (!dll)
			return NULL;

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

	bool IsInvalidPath(const string_t& strPath)
	{
#if defined(_WIN32)
		wchar_t wpath[MAX_PATH] = {0};
		if (MultiByteToWideChar(CP_UTF8,0,strPath.c_str(),-1,wpath,MAX_PATH-1) <= 0)
			return true;

		return (PathIsRelativeW(wpath) != FALSE);
#else
		// Allow PATH-based paths
		return (strPath[0] != '/' && strPath.Find('/') != string_t::npos);
#endif
	}

	ObjectPtr<Registry::IKey> GetObjectsKey(const string_t& strSubKey)
	{
		ObjectPtr<Registry::IKey> ptrObjects = ObjectPtr<Registry::IOverlayKeyFactory>(Registry::OID_OverlayKeyFactory,Activation::Process | Activation::DontLaunch)->Overlay(string_t::constant("Local User/Objects"),string_t::constant("All Users/Objects"));
		return ptrObjects->OpenKey(strSubKey);
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

	IObject* LoadSurrogateObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid, bool wrong_platform)
	{
		string_t strOid = string_t::constant("Omega.Surrogate");

		Activation::Flags_t sgt_flags = (flags & 0xF);
		if (sgt_flags < Activation::Process)
			sgt_flags = Activation::Process;

		// Always launch the surrogate as an app - this protects against in-process surrogates!
		IObject* pObject = NULL;
		OOCore::GetInterProcessService()->LaunchObjectApp(NameToOid(strOid),OMEGA_GUIDOF(Activation::IObjectFactory),sgt_flags,pObject);
		ObjectPtr<Activation::IObjectFactory> ptrOF = static_cast<Activation::IObjectFactory*>(pObject);

		pObject = NULL;
		ptrOF->CreateInstance(OMEGA_GUIDOF(Remoting::ISurrogate),pObject);
		ObjectPtr<Remoting::ISurrogate> ptrSurrogate = static_cast<Remoting::ISurrogate*>(pObject);

		pObject = NULL;
		ptrSurrogate->GetObject(oid,flags,iid,pObject);
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
			if (strLib.IsEmpty() || IsInvalidPath(strLib))
				throw IAccessDeniedException::Create(OOCore::get_text("Invalid path \"{0}\" in library registry value.") % strLib);
		}
		
		if (!strLib.IsEmpty())
		{
			// Try to load a library, if allowed
			bool wrong_platform = false;
			if (sub_type == Activation::Default || sub_type == Activation::Library)
			{
				// Load the library locally
				IObject* pObj = LoadLibraryObject(strLib,oid,iid,(sub_type == Activation::Default));
				if (pObj)
					return pObj;

				wrong_platform = true;
			}
			
			// Run a surrogate
			return LoadSurrogateObject(oid,flags,iid,wrong_platform);
		}

		if (sub_type == Activation::Library)
		{
			// Force an exception
			ptrOidKey->GetValue(Omega::string_t::constant("Library")).cast<string_t>();
		}

		IObject* pObject = NULL;
		OOCore::GetInterProcessService()->LaunchObjectApp(oid,iid,flags,pObject);
		return pObject;
	}

	IObject* GetLocalInstance(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid)
	{
		// See if we have it registered in the ROT
		IObject* pObject = OOCore::GetRegisteredObject(oid,iid);
		if (!pObject)
		{
			// See if we are allowed to load...
			if (flags & Activation::DontLaunch)
				throw INotFoundException::Create(OOCore::get_text("A library or running instance of object {0} could not be found") % oid);

			pObject = LoadObject(oid,flags,iid);
		}

		return pObject;
	}
}

template class Threading::Singleton<DLLManagerImpl,Threading::InitialiseDestructor<OOCore::DLL> >;

DLLManagerImpl::~DLLManagerImpl()
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	// Clear out our map now, as the smart ptrs use m_lock
	m_dll_map.clear();
}

OOBase::SmartPtr<OOBase::DLL> DLLManagerImpl::load_dll(const string_t& name, bool allow_null)
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

#if defined(_WIN32)
	if (allow_null && err == ERROR_BAD_EXE_FORMAT)
		return OOBase::SmartPtr<OOBase::DLL>();	
#endif

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
	// This may skip a few entries, but it makes best effort

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

bool DLLManagerImpl::can_unload()
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	for (size_t i=0; i<m_dll_map.size(); ++i)
	{
		OOBase::SmartPtr<OOBase::DLL> dll = *m_dll_map.at(i);

		try
		{
			pfnCanUnloadLibrary pfn = (pfnCanUnloadLibrary)(dll->symbol("Omega_CanUnloadLibrary_Safe"));
			if (!pfn)
				return false;

			bool unload = false;
			System::Internal::SafeShim* CanUnloadLibrary_Exception = pfn(System::Internal::marshal_info<bool_t&>::safe_type::coerce(unload));

			// Ignore exceptions
			if (CanUnloadLibrary_Exception)
				System::Internal::release_safe(CanUnloadLibrary_Exception);

			if (!unload)
				return false;
		}
		catch (IException* pE)
		{
			// Ignore exceptions
			pE->Release();
			return false;
		}
	}

	return true;
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

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_GetInstance,4,((in),const Omega::any_t&,oid,(in),Omega::Activation::Flags_t,flags,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject))
{
	pObject = OOCore::GetInstance(oid,flags,iid);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool_t,OOCore_Omega_CanUnload,0,())
{
	return DLLManager::instance()->can_unload();
}
