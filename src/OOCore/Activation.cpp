#include "OOCore_precomp.h"

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	Activation::IObjectFactory* LoadObjectLibrary(const string_t& dll_name, const guid_t& oid, Activation::Flags_t flags);
	void ExecProcess(const string_t& strExeName);

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
		static void Throw(const string_t& strName, IException* pE = 0);

		BEGIN_INTERFACE_MAP(LibraryNotFoundExceptionImpl)
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

void OOCore::LibraryNotFoundExceptionImpl::Throw(const string_t& strName, IException* pE)
{
	ObjectImpl<OOCore::LibraryNotFoundExceptionImpl>* pRE = ObjectImpl<OOCore::LibraryNotFoundExceptionImpl>::CreateObject();
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
			LibraryNotFoundExceptionImpl::Throw(dll_name);

		typedef MetaInfo::IException_Safe* (OMEGA_CALL *pfnGetObjectFactory)(MetaInfo::interface_info<Activation::IObjectFactory*&>::safe_class pOF, MetaInfo::interface_info<const guid_t&>::safe_class oid, MetaInfo::interface_info<Activation::Flags_t>::safe_class flags);
		pfnGetObjectFactory pfn = (pfnGetObjectFactory)dll.symbol(ACE_TEXT("Omega_GetObjectFactory_Safe"));
		if (pfn == 0)
			OOCORE_THROW_LASTERROR();

		ObjectPtr<Activation::IObjectFactory> ptrOF;
		MetaInfo::IException_Safe* GetObjectFactory_Exception = pfn(
			MetaInfo::interface_info<Activation::IObjectFactory*&>::proxy_functor(ptrOF),
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

void OOCore::ExecProcess(const string_t& strExeName)
{
	// Set the process options
	ACE_Process_Options options;
	options.avoid_zombies(0);
	options.handle_inheritence(0);
	if (options.command_line(strExeName) == -1)
		OOCORE_THROW_ERRNO(ACE_OS::last_error() ? ACE_OS::last_error() : EINVAL);

	// Set the creation flags
	u_long flags = 0;
#if defined (ACE_WIN32)
	flags |= CREATE_NEW_CONSOLE;
#endif
	options.creation_flags(flags);

	// Spawn the process
	ACE_Process process;
	if (process.spawn(options)==ACE_INVALID_PID)
		OOCORE_THROW_LASTERROR();

	void* TODO;	// Make this a smarter countdown timer

	// Wait 1 second for the process to launch, if it takes more than 1 second its probably okay
	ACE_exitcode exitcode = 0;
	int ret = process.wait(ACE_Time_Value(1),&exitcode);
	if (ret==-1)
		OOCORE_THROW_LASTERROR();

	if (ret!=0)
		OOCORE_THROW_ERRNO(ret);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Activation_IOidNotFoundException_Throw,2,((in),const guid_t&,oid,(in),IException*,pE))
{
	ObjectImpl<OOCore::OidNotFoundExceptionImpl>* pNew = ObjectImpl<OOCore::OidNotFoundExceptionImpl>::CreateObject();
	pNew->m_strDesc = "OID not found.";
	pNew->m_ptrCause = pE;
	pNew->m_oid = oid;
	throw pNew;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Activation_INoAggregationException_Throw,2,((in),const guid_t&,oid,(in),IException*,pE))
{
	ObjectImpl<OOCore::NoAggregationExceptionImpl>* pNew = ObjectImpl<OOCore::NoAggregationExceptionImpl>::CreateObject();
	pNew->m_strDesc = "Object does not supported aggregation.";
	pNew->m_ptrCause = pE;
	pNew->m_oid = oid;
	throw pNew;
}

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

	Registry::INotFoundException::Throw(strObjectName,OMEGA_SOURCE_INFO);
	return guid_t::NIL;
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
					// Launch the server - Do something cleverer here at some point
					ObjectPtr<Registry::IRegistryKey> ptrServer("Applications/" + ptrOidKey->GetStringValue("Application") + "/Activation");

					OOCore::ExecProcess(ptrServer->GetStringValue("Exec"));

					ObjectPtr<Activation::IServiceTable> ptrServiceTable;
					ptrServiceTable.Attach(Activation::IServiceTable::GetServiceTable());

					// Wait for startup
					ACE_Time_Value wait(5);

					// TODO The timeout needs to be related to the request timeout...
					void* TODO;

					ACE_Countdown_Time timeout(&wait);
					while (!timeout.stopped())
					{
						// Change this to use monikers one day!
						IObject* pObject = 0;
						ptrServiceTable->GetObject(oid,Activation::IID_IObjectFactory,pObject);
						if (pObject)
							return static_cast<Activation::IObjectFactory*>(pObject);

						timeout.update();
					}
				}
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
	return 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Activation_CreateObject,5,((in),const guid_t&,oid,(in),Activation::Flags_t,flags,(in),IObject*,pOuter,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
{
	ObjectPtr<Activation::IObjectFactory> ptrOF;
	ptrOF.Attach(Activation_GetObjectFactory_Impl(oid,flags));
	ptrOF->CreateObject(pOuter,iid,pObject);
}
