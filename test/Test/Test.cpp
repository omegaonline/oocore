#include "./Test.h"

#ifdef ACE_WIN32
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
#if defined (ACE_DISABLES_THREAD_LIBRARY_CALLS) && (ACE_DISABLES_THREAD_LIBRARY_CALLS == 1)
	if (reason == DLL_PROCESS_ATTACH)
		::DisableThreadLibraryCalls(instance);
#endif /* ACE_DISABLES_THREAD_LIBRARY_CALLS */
	
	return TRUE;
}
#endif
