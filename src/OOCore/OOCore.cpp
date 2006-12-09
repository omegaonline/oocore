#include "OOCore_precomp.h"

#include "./StdObjectManager.h"
#include "./StdApartment.h"
#include "./Session.h"

using namespace Omega;
using namespace OTL;

// Our library map
BEGIN_LIBRARY_OBJECT_MAP(OOCore)
	OBJECT_MAP_ENTRY(StdObjectManager)
END_LIBRARY_OBJECT_MAP()

#if defined(OMEGA_WIN32)
BOOL WINAPI DllMain(HINSTANCE /*instance*/, DWORD reason)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
#if defined (ACE_DISABLES_THREAD_LIBRARY_CALLS) && (ACE_DISABLES_THREAD_LIBRARY_CALLS == 1)
		::DisableThreadLibraryCalls(instance);
#endif /* ACE_DISABLES_THREAD_LIBRARY_CALLS */

		ModuleInitialize();
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		ModuleUninitialize();
	}

	return TRUE;
}
#endif

struct OOThreadContext
{
	OOThreadContext() :
		m_initcount(0), m_pApartment(0)
	{}

	AtomicOp<long>::type						m_initcount;
	AtomicOp<Activation::IApartment*>::type		m_pApartment;

	// Extra per-thread security based stuff should probably go here one day!
};

namespace 
{
	ACE_TSS<OOThreadContext>				s_thread_context;
	AtomicOp<long>::type					s_initcount = 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Activation::IApartment*,IApartment_GetCurrentApartment,0,())
{
	Activation::IApartment* pApartment = s_thread_context->m_pApartment.value();
	pApartment->AddRef();
	return pApartment;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,Omega_Initialize,1,((in),Activation::IApartment*,pApartment))
{
	bool bStart = false;
	if (++s_initcount==1)
	{
		// Call ACE::init() first
		bStart = true;

		int ret = ACE::init();
		if (ret == 1)
			ret = 0;

		if (ret != 0)
		{
			--s_initcount;
			ObjectImpl<ExceptionImpl<IException> >* pE = ObjectImpl<ExceptionImpl<IException> >::CreateObject();
			pE->m_strDesc = ACE_OS::strerror(ACE_OS::last_error());
			return pE;
		}
	}

	if (++s_thread_context->m_initcount == 1)
	{
		try
		{
			// Set the apartment...
			Activation::IApartment* pNew = pApartment;
			if (pNew == (Activation::IApartment*)-1 ||
				pNew == 0)
			{
				pNew = ObjectImpl<StdApartment>::CreateObject();
			}
			else
			{
				pNew->AddRef();
			}

			s_thread_context->m_pApartment.exchange(pNew);
		}
		catch (IException* pE)
		{
			--s_thread_context->m_initcount;
			return pE;
		}
	}

	if (bStart && pApartment != (Activation::IApartment*)-1)
	{
		ObjectPtr<IException> ptrE;
		try
		{
			// Now connect to our local server
			Session::Connect();
		}
		catch (IException* pE)
		{
			ptrE.Attach(pE);
		}
		catch (std::exception& e)
		{
			ObjectImpl<ExceptionImpl<IException> >* pE = ObjectImpl<ExceptionImpl<IException> >::CreateObject();
			pE->m_strDesc = e.what();
			ptrE.Attach(pE);
		}
		catch (...)
		{
			ObjectImpl<ExceptionImpl<IException> >* pE = ObjectImpl<ExceptionImpl<IException> >::CreateObject();
			pE->m_strDesc = ACE_OS::strerror(ACE_OS::last_error());
            ptrE.Attach(pE);
		}

		if (ptrE)
		{
			try
			{
				s_thread_context->m_pApartment.exchange(0)->Release();
			}
			catch (IException* pE)
			{
				pE->Release();
			}
			catch (...)
			{ }
			
			ACE::fini();

			return ptrE.AddRefReturn();
		}
	}

	return 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_Initialize_Minimal,0,())
{
	Omega_Initialize_Impl((Activation::IApartment*)-1);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_Uninitialize,0,())
{
	if (--s_thread_context->m_initcount == 0)
	{
		try 
		{ 
			s_thread_context->m_pApartment.exchange(0)->Release();
		} 
		catch (IException* pE)
		{
			pE->Release();
		}
		catch (...) 
		{}
	}

	if (--s_initcount==0)
	{
		/*try 
		{ 
			//LocalTransport::term();
		} 
		catch (IException* pE)
		{
			pE->Release();
		}
		catch (...) 
		{}*/

		ACE::fini();
	}
}


// Helpers
void ExecProcess(const string_t& strExeName)
{
	// Set the process options
	ACE_Process_Options options;
	options.avoid_zombies(0);
	options.handle_inheritence(0);
	if (options.command_line(strExeName) == -1)
		OOCORE_THROW_ERRNO(ACE_OS::last_error() ? ACE_OS::last_error() : EINVAL);

	// Set the creation flags
	u_long flags = 0;
#if defined (OMEGA_WIN32)
	flags |= CREATE_NEW_CONSOLE;
#endif
	options.creation_flags(flags);

	// Spawn the process
	ACE_Process process;
	if (process.spawn(options)==ACE_INVALID_PID)
		OOCORE_THROW_LASTERROR();

	// Wait 1 second for the process to launch, if it takes more than 1 second its probably okay
	ACE_exitcode exitcode = 0;
	int ret = process.wait(ACE_Time_Value(1),&exitcode);
	if (ret==-1)
		OOCORE_THROW_LASTERROR();

	if (ret!=0)
		OOCORE_THROW_ERRNO(ret);
}
