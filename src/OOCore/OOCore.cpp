#include "OOCore_precomp.h"

#include "./UserSession.h"

#include "./StdObjectManager.h"

using namespace Omega;
using namespace OTL;

// Our library map
BEGIN_LIBRARY_OBJECT_MAP(OOCore)
	OBJECT_MAP_ENTRY(StdObjectManager)
END_LIBRARY_OBJECT_MAP()

#if defined(ACE_WIN32)
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

static AtomicOp<long>::type	s_initcount = 0;

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,Omega_Initialize,0,())
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

	if (bStart)
	{
		IException* pE = UserSession::init();
		if (pE)
		{
			return pE;
		}
	}

	return 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_Initialize_Minimal,0,())
{
	Omega_Initialize_Impl();
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_Uninitialize,0,())
{
	if (--s_initcount==0)
	{
		UserSession::term();

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
#if defined (ACE_WIN32)
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
