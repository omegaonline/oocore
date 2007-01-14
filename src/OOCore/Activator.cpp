#include "OOCore_precomp.h"

using namespace Omega;
using namespace OTL;

void ExecProcess(const string_t& strExeName);

void Omega_GetObjectFactory_Impl(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject);

class ActivationImpl
{
public:
	typedef MetaInfo::IException_Safe* (OMEGA_CALL *pfnGetObjectFactory)(MetaInfo::interface_info<const guid_t&>::safe_class oid, MetaInfo::interface_info<Activation::Flags_t>::safe_class flags, MetaInfo::interface_info<const guid_t&>::safe_class iid, MetaInfo::interface_info<IObject*&>::safe_class pObject);
	typedef ACE_DLL_Singleton_T<ActivationImpl,ACE_Recursive_Thread_Mutex> ACTIVATOR;

	ActivationImpl();

	const ACE_TCHAR* name();
	const ACE_TCHAR* dll_name();
	
	IObject* GetObjectFactory(const string_t& dll_name, const guid_t& oid, Activation::Flags_t flags, const guid_t& iid);
};

class OidNotFoundExceptionImpl :
	public ExceptionImpl<Activation::IOidNotFoundException>
{
public:
	guid_t					m_oid;

	BEGIN_INTERFACE_MAP(OidNotFoundExceptionImpl)
		INTERFACE_ENTRY_CHAIN(ExceptionImpl<Activation::IOidNotFoundException>)
	END_INTERFACE_MAP()

// Activation::IOidNotFoundException members
public:
	guid_t GetMissingOid()
	{
		return m_oid;
	}
};

class NoAggregationExceptionImpl :
	public ExceptionImpl<Activation::INoAggregationException>
{
public:
	guid_t					m_oid;

	BEGIN_INTERFACE_MAP(NoAggregationExceptionImpl)
		INTERFACE_ENTRY_CHAIN(ExceptionImpl<Activation::INoAggregationException>)
	END_INTERFACE_MAP()

// Activation::INoAggregationException members
public:
	guid_t GetFailingOid()
	{
		return m_oid;
	}
};

class LibraryNotFoundExceptionImpl :
	public ExceptionImpl<Activation::ILibraryNotFoundException>
{
public:
	string_t m_dll_name;

	BEGIN_INTERFACE_MAP(LibraryNotFoundExceptionImpl)
		INTERFACE_ENTRY_CHAIN(ExceptionImpl<Activation::ILibraryNotFoundException>)
	END_INTERFACE_MAP()

// Activation::ILibraryNotFoundException members
public:
	string_t GetLibraryName()
	{
		return m_dll_name;
	}
};

void ILibraryNotFoundException_Throw(const string_t& strName, IException* pE = 0)
{
	ObjectImpl<LibraryNotFoundExceptionImpl>* pRE = ObjectImpl<LibraryNotFoundExceptionImpl>::CreateObject();
	pRE->m_ptrCause = pE;
	pRE->m_strDesc = string_t::Format("Dynamic library '%s' not found",static_cast<const char_t*>(strName));
	pRE->m_dll_name = strName;
	throw pRE;
}

ActivationImpl::ActivationImpl()
{
	// Ensure we are using per-dll unloading
	ACE_DLL_Manager::instance()->unload_policy(ACE_DLL_UNLOAD_POLICY_PER_DLL);
}

const ACE_TCHAR* 
ActivationImpl::name()
{
	return ACE_TEXT("ActivationImpl");
}

const ACE_TCHAR* 
ActivationImpl::dll_name()
{
	return ACE_TEXT("OOCore");
}

IObject* ActivationImpl::GetObjectFactory(const string_t& dll_name, const guid_t& oid, Activation::Flags_t flags, const guid_t& iid)
{
	ACE_DLL dll;
	if (dll_name != "OOCore")
	{
        if (dll.open(ACE_TEXT_CHAR_TO_TCHAR(dll_name)) != 0)
			ILibraryNotFoundException_Throw(dll_name);

		pfnGetObjectFactory pfn = (pfnGetObjectFactory)dll.symbol(ACE_TEXT("Omega_GetObjectFactory_Safe"));
		if (pfn == 0)
			OOCORE_THROW_LASTERROR();

		ObjectPtr<IObject> ptrObject;
		MetaInfo::IException_Safe* GetObjectFactory_Exception = pfn(MetaInfo::interface_info<const guid_t&>::proxy_functor(oid), MetaInfo::interface_info<Activation::Flags_t>::proxy_functor(flags), MetaInfo::interface_info<const guid_t&>::proxy_functor(iid), MetaInfo::interface_info<IObject**>::proxy_functor(&ptrObject,iid)); 
		if (GetObjectFactory_Exception) 
			MetaInfo::throw_correct_exception(GetObjectFactory_Exception); 
		return ptrObject.AddRefReturn(); 
	}
	else
	{
		// Its us!
		IObject* pObject = 0;
		Omega_GetObjectFactory_Impl(oid,flags,iid,pObject);
		return pObject; 
	}
}

#if (defined(_MSC_VER) && _MSC_VER>=1300)
// These functions contain unreachable code, which we know about, so shut up the warning
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Activation_IOidNotFoundException_Throw,2,((in),const guid_t&,oid,(in),IException*,pE))
{
	ObjectImpl<OidNotFoundExceptionImpl>* pNCE = ObjectImpl<OidNotFoundExceptionImpl>::CreateObject();
	pNCE->m_strDesc = "OID not found.";
	pNCE->m_ptrCause = pE;
	pNCE->m_oid = oid;
	throw pNCE;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Activation_INoAggregationException_Throw,2,((in),const guid_t&,oid,(in),IException*,pE))
{
	ObjectImpl<NoAggregationExceptionImpl>* pNCE = ObjectImpl<NoAggregationExceptionImpl>::CreateObject();
	pNCE->m_strDesc = "Object does not supported aggregation.";
	pNCE->m_ptrCause = pE;
	pNCE->m_oid = oid;
	throw pNCE;
}

#if (defined(_MSC_VER) && _MSC_VER>=1300)
// These functions contain unreachable code, which we know about, so shut up the warning
#pragma warning(pop)
#endif

OMEGA_DEFINE_EXPORTED_FUNCTION(guid_t,Activation_NameToOid,1,((in),const string_t&,strObjectName))
{
	string_t strCurName = strObjectName;

	for (int i=0;i<2;++i)
	{
		ObjectPtr<Registry::IRegistryKey> ptrOidKey("Objects/" + strCurName);

		if (ptrOidKey->IsValue("OID"))
			return guid_t::FromString(ptrOidKey->GetStringValue("OID"));
				
		strCurName = ptrOidKey->GetStringValue("CurrentVersion");
	}

	Registry::INotFoundException::Throw(strObjectName,OMEGA_FUNCNAME);
	return guid_t::NIL;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Activation_GetObjectFactory,4,((in),const guid_t&,oid,(in),Activation::Flags_t,flags,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
{
	try
	{
		pObject = 0;

		// Try ourselves first...
		if (flags & Activation::InProcess)
		{
			Omega_GetObjectFactory_Impl(oid,flags,iid,pObject);
			if (pObject)
				return;
		}

		ObjectPtr<Registry::IRegistryKey> ptrOidsKey("Objects/OIDs");

		// Look in-process first if requested and available
		if ((flags & Activation::InProcess) && ptrOidsKey->IsSubKey(oid))
		{
			ObjectPtr<Registry::IRegistryKey> ptrOidKey = ptrOidsKey.OpenSubKey(oid);
			if (ptrOidKey->IsValue("Library"))
			{
				pObject = ActivationImpl::ACTIVATOR::instance()->GetObjectFactory(ptrOidKey->GetStringValue("Library"),oid,flags,iid);
				if (pObject)
					return;
			}			
		}
		
		if (flags & Activation::OutOfProcess)
		{
			// Do twice
			for (int i=0;i<2;++i)
			{
				// If we aren't the server then, ask the local server
				ObjectPtr<Activation::IRunningObjectTable> ptrROT;
				ptrROT.Attach(Activation::IRunningObjectTable::GetRunningObjectTable());
				
				// Change this to use monikers one day!
				ptrROT->GetRegisteredObject(oid,iid,pObject);
				if (pObject)
					return;
				
				// If we have been told not to launch, break out of the loop
				if (flags & Activation::DontLaunch || !ptrOidsKey->IsSubKey(oid))
					break;

				// Launch the server - Do something cleverer here at some point
				ObjectPtr<Registry::IRegistryKey> ptrOidKey = ptrOidsKey.OpenSubKey(oid);
				ObjectPtr<Registry::IRegistryKey> ptrServer("Applications/" + ptrOidKey->GetStringValue("Application") + "/Activation");
			
				ExecProcess(ptrServer->GetStringValue("Exec"));
			}
		}
	}
	catch (IException* pE)
	{
		ObjectPtr<IException> ptrE;
		ptrE.Attach(pE);

		Activation::IOidNotFoundException::Throw(oid,ptrE);
	}
			
	Activation::IOidNotFoundException::Throw(oid);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Activation_CreateObject,5,((in),const guid_t&,oid,(in),Activation::Flags_t,flags,(in),IObject*,pOuter,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
{
	IObject* pObj = 0;
	Activation_GetObjectFactory_Impl(oid,flags,Activation::IID_IObjectFactory,pObj);
	ObjectPtr<Activation::IObjectFactory> ptrOF;
	ptrOF.Attach(static_cast<Activation::IObjectFactory*>(pObj));
	ptrOF->CreateObject(pOuter,iid,pObject);
}
