#include "OOCore_precomp.h"

#include "./UserSession.h"

#include "./StdObjectManager.h"

using namespace Omega;
using namespace OTL;

// Our library map
BEGIN_LIBRARY_OBJECT_MAP(OOCore)
	OBJECT_MAP_ENTRY(OOCore::StdObjectManager)
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

namespace OOCore
{
	static AtomicOp<long>::type	s_initcount = 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,Omega_Initialize,0,())
{
	bool bStart = false;
	if (++OOCore::s_initcount==1)
	{
		// Call ACE::init() first
		bStart = true;

		int ret = ACE::init();
		if (ret == 1)
			ret = 0;

		if (ret != 0)
		{
			--OOCore::s_initcount;
			ObjectImpl<ExceptionImpl<IException> >* pE = ObjectImpl<ExceptionImpl<IException> >::CreateObject();
			pE->m_strDesc = ACE_OS::strerror(ACE_OS::last_error());
			pE->m_strSource = OMEGA_SOURCE_INFO;
			return pE;
		}
	}

	if (bStart)
	{
		IException* pE = OOCore::UserSession::init();
		if (pE)
		{
			return pE;
		}
	}

	return 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_Uninitialize,0,())
{
	if (--OOCore::s_initcount==0)
	{
		OOCore::UserSession::term();

		ACE::fini();
	}
}
