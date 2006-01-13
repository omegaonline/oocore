#include "./Test.h"

DEFINE_IID(Test::Test,6AAE8C33-699A-4414-AF84-25E74E693207);
DEFINE_CLSID(Test,7A5701A9-28FD-4fa0-8D95-77D00C753444);

#ifdef ACE_WIN32
BEGIN_META_INFO_MAP(Test)
#else
BEGIN_META_INFO_MAP_EX(Test,OOTest)
#endif
	META_INFO_ENTRY(Test::Test)
END_META_INFO_MAP()

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