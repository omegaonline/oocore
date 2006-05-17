#include "./OOObject.h"

#include "./Guid.h"

namespace OOCore
{
namespace Impl
{
	OOCore_Export int RegisterAsServer();
	
	bool g_IsServer = false;
};
};

#ifdef ACE_WIN32
namespace OOCore
{
namespace Impl
{
	HINSTANCE g_hInstance = 0;
};
};

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		OOCore::Impl::g_hInstance = instance;

#if defined (ACE_DISABLES_THREAD_LIBRARY_CALLS) && (ACE_DISABLES_THREAD_LIBRARY_CALLS == 1)
		::DisableThreadLibraryCalls(instance);
#endif /* ACE_DISABLES_THREAD_LIBRARY_CALLS */

	}
	
	return TRUE;
}
#endif

OOCore_Export int 
OOCore::Impl::RegisterAsServer()
{
	// This is the only place this is set!
	g_IsServer = true;
	
	return 0;
}

