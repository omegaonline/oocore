#include "OOCore_precomp.h"

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	Activation::IObjectFactory* LoadObjectLibrary(const string_t& dll_name, const guid_t& oid, Activation::Flags_t flags);
	void ExecProcess(ACE_Process& process, const string_t& strExeName);
	ACE_CString ShellParse(const char* pszFile);
    
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
		guid_t					m_oid;

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
}

void OOCore::LibraryNotFoundException::Throw(const string_t& strName, IException* pE)
{
	ObjectImpl<OOCore::LibraryNotFoundException>* pRE = ObjectImpl<OOCore::LibraryNotFoundException>::CreateInstance();
	pRE->m_ptrCause = pE;
	pRE->m_strDesc = string_t::Format("Dynamic library '%s' not found",static_cast<const char_t*>(strName));
	pRE->m_dll_name = strName;
	throw pRE;
}

// External declaration of our version of this entry point
Activation::IObjectFactory* Omega_GetObjectFactory_Impl(const guid_t& oid, Activation::Flags_t flags);

Activation::IObjectFactory* OOCore::LoadObjectLibrary(const string_t& dll_name, const guid_t& oid, Activation::Flags_t flags)
{
	ACE_DLL dll;
	if (dll_name != "OOCore")
	{
		// Ensure we are using per-dll unloading
		ACE_DLL_Manager::instance()->unload_policy(ACE_DLL_UNLOAD_POLICY_PER_DLL);

        if (dll.open(ACE_TEXT_CHAR_TO_TCHAR(dll_name)) != 0)
			LibraryNotFoundException::Throw(dll_name);

		typedef MetaInfo::IException_Safe* (OMEGA_CALL *pfnGetObjectFactory)(MetaInfo::interface_info<Activation::IObjectFactory*&>::safe_class pOF, MetaInfo::interface_info<const guid_t&>::safe_class oid, MetaInfo::interface_info<Activation::Flags_t>::safe_class flags);
		pfnGetObjectFactory pfn = (pfnGetObjectFactory)dll.symbol(ACE_TEXT("Omega_GetObjectFactory_Safe"));
		if (pfn == 0)
			OOCORE_THROW_LASTERROR();

		ObjectPtr<Activation::IObjectFactory> ptrOF;
		MetaInfo::IException_Safe* GetObjectFactory_Exception = pfn(
			MetaInfo::interface_info<Activation::IObjectFactory* volatile &>::proxy_functor(ptrOF),
			MetaInfo::interface_info<const guid_t&>::proxy_functor(oid),
			MetaInfo::interface_info<Activation::Flags_t>::proxy_functor(flags));

		if (GetObjectFactory_Exception)
			MetaInfo::throw_correct_exception(GetObjectFactory_Exception);
		return ptrOF.Detach();
	}
	else
	{
		// Its us!
		return Omega_GetObjectFactory_Impl(oid,flags);
	}
}

ACE_CString OOCore::ShellParse(const char* pszFile)
{
	ACE_CString strRet = pszFile;

#if defined(OMEGA_WIN32)
		
	const char* pszExt = PathFindExtensionA(pszFile);
	if (pszExt)
	{
		DWORD dwLen = 1024;
		char szBuf[1024];	
		HRESULT hRes = AssocQueryStringA(ASSOCF_NOTRUNCATE | ASSOCF_REMAPRUNDLL,ASSOCSTR_COMMAND,pszExt,NULL,szBuf,&dwLen);
		if (hRes == S_OK)
			strRet = szBuf;
		else if (hRes == E_POINTER)
		{
			char* pszBuf = new char[dwLen+1];
			hRes = AssocQueryStringA(ASSOCF_NOTRUNCATE | ASSOCF_REMAPRUNDLL,ASSOCSTR_COMMAND,pszExt,NULL,pszBuf,&dwLen);
			if (hRes==S_OK)
				strRet = pszBuf;

			delete [] pszBuf;
		}

		if (hRes == S_OK)
		{
			LPVOID lpBuffer = 0;
			if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
				strRet.c_str(),0,0,(LPSTR)&lpBuffer,0,(va_list*)&pszFile))
			{
				strRet = (LPSTR)lpBuffer;
				LocalFree(lpBuffer);
			}
		}	
	}

#endif

	return strRet;
}

void OOCore::ExecProcess(ACE_Process& process, const string_t& strExeName)
{
	// Set the process options 
	ACE_Process_Options options(0);
	options.avoid_zombies(0);
	options.handle_inheritence(0);

	// Do a ShellExecute style lookup for the actual thing to call..
	ACE_CString strActualName = ShellParse(strExeName);

	if (options.command_line(strActualName.c_str()) == -1)
		OOCORE_THROW_ERRNO(ACE_OS::last_error() ? ACE_OS::last_error() : EINVAL);

	// Set the creation flags
	u_long flags = 0;
#if defined(OMEGA_WIN32)
	flags |= CREATE_NEW_CONSOLE;
#endif
	options.creation_flags(flags);

	// Spawn the process
	if (process.spawn(options)==ACE_INVALID_PID)
		OOCORE_THROW_LASTERROR();
}

void OOCore::OidNotFoundException::Throw(const guid_t& oid, IException* pE)
{
	ObjectImpl<OOCore::OidNotFoundException>* pNew = ObjectImpl<OOCore::OidNotFoundException>::CreateInstance();
	pNew->m_strDesc = "The identified object could not be found.";
	pNew->m_ptrCause = pE;
	pNew->m_oid = oid;
	throw pNew;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Activation_INoAggregationException_Throw,2,((in),const guid_t&,oid,(in),IException*,pE))
{
	ObjectImpl<OOCore::NoAggregationException>* pNew = ObjectImpl<OOCore::NoAggregationException>::CreateInstance();
	pNew->m_strDesc = "Object does not supported aggregation.";
	pNew->m_ptrCause = pE;
	pNew->m_oid = oid;
	throw pNew;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(guid_t,Activation_NameToOid,1,((in),const string_t&,strObjectName))
{
	string_t strCurName = strObjectName;
	for (;;)
	{
		ObjectPtr<Registry::IRegistryKey> ptrOidKey("Objects\\" + strCurName);

		if (ptrOidKey->IsValue("OID"))
			return guid_t::FromString(ptrOidKey->GetStringValue("OID"));

		strCurName = ptrOidKey->GetStringValue("CurrentVersion");
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Activation::IObjectFactory*,Activation_GetObjectFactory,2,((in),const guid_t&,oid,(in),Activation::Flags_t,flags))
{
	try
	{
		// Try ourselves first...
		if (flags & Activation::InProcess)
		{
			Activation::IObjectFactory* pOF = Omega_GetObjectFactory_Impl(oid,flags);
			if (pOF)
				return pOF;
		}

		// Try ServiceTable if possible
		if (flags & Activation::OutOfProcess)
		{
			ObjectPtr<Activation::IServiceTable> ptrServiceTable;
			ptrServiceTable.Attach(Activation::IServiceTable::GetServiceTable());

			// Change this to use monikers one day!
			IObject* pObject = 0;
			ptrServiceTable->GetObject(oid,Activation::IID_IObjectFactory,pObject);
			if (pObject)
				return static_cast<Activation::IObjectFactory*>(pObject);
		}

		if (!(flags & Activation::DontLaunch))
		{
			// Use the registry
			ObjectPtr<Registry::IRegistryKey> ptrOidsKey("Objects/OIDs");
			if (ptrOidsKey->IsSubKey(oid))
			{
				ObjectPtr<Registry::IRegistryKey> ptrOidKey = ptrOidsKey.OpenSubKey(oid);

				if (flags & Activation::InProcess)
				{
					if (ptrOidKey->IsValue("Library"))
						return OOCore::LoadObjectLibrary(ptrOidKey->GetStringValue("Library"),oid,flags);
				}

				if (flags & Activation::OutOfProcess)
				{
					// Get the service table
					ObjectPtr<Activation::IServiceTable> ptrServiceTable;
					ptrServiceTable.Attach(Activation::IServiceTable::GetServiceTable());

					// Find the name of the executeable to run
					ObjectPtr<Registry::IRegistryKey> ptrServer("Applications/" + ptrOidKey->GetStringValue("Application") + "/Activation");

					// Launch the executeable
					ACE_Process process;
					OOCore::ExecProcess(process,ptrServer->GetStringValue("Exec"));

					// TODO The timeout needs to be related to the request timeout...
					void* TODO;

					// Wait for startup
					ACE_Time_Value wait(15);
					ACE_Countdown_Time timeout(&wait);
					while (wait != ACE_Time_Value::zero)
					{
						// Change this to use monikers one day!
						IObject* pObject = 0;
						ptrServiceTable->GetObject(oid,Activation::IID_IObjectFactory,pObject);
						if (pObject)
							return static_cast<Activation::IObjectFactory*>(pObject);

						// Check if the process is still running...
						if (!process.running())
							break;

						// Update our countdown
						timeout.update();
					}

					// Kill the process - TODO - Do we *really* want to do this?
					process.kill();
				}
			}
		}
	}
	catch (IException* pE)
	{
		ObjectPtr<IException> ptrE;
		ptrE.Attach(pE);

		OOCore::OidNotFoundException::Throw(oid,ptrE);
	}

	OOCore::OidNotFoundException::Throw(oid);
	return 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Activation_CreateInstance,5,((in),const guid_t&,oid,(in),Activation::Flags_t,flags,(in),IObject*,pOuter,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
{
	ObjectPtr<Activation::IObjectFactory> ptrOF;
	ptrOF.Attach(Activation_GetObjectFactory_Impl(oid,flags));
	ptrOF->CreateInstance(pOuter,iid,pObject);
}
