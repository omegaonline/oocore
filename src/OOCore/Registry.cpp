#include "OOCore_precomp.h"

#if defined(OMEGA_WIN32)
// For the Windows path functions
#include <shlwapi.h>
#include <shlobj.h>
#endif

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	// The instance wide ServiceTable instance
	struct Registry
	{
		ObjectPtr<Omega::Registry::IRegistryKey>  m_ptrRegistryRoot;
		ACE_Thread_Mutex                          m_lock;
	};
	Registry	g_Registry;
}

void SetRegistry(Registry::IRegistryKey* pRootKey)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,OOCore::g_Registry.m_lock,OOCORE_THROW_LASTERROR());

	OOCore::g_Registry.m_ptrRegistryRoot = pRootKey;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Registry::IRegistryKey*,IRegistryKey_OpenKey,2,((in),const string_t&,key,(in),Registry::IRegistryKey::OpenFlags_t,flags))
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,OOCore::g_Registry.m_lock,OOCORE_THROW_LASTERROR());

	return OOCore::g_Registry.m_ptrRegistryRoot->OpenSubKey(key,flags);
}
