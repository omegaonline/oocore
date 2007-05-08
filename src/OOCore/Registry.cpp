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
	ObjectPtr<Omega::Registry::IRegistryKey>  g_ptrRegistryRoot;
	
	void SetRegistry(Omega::Registry::IRegistryKey* pRootKey);
}

void OOCore::SetRegistry(Omega::Registry::IRegistryKey* pRootKey)
{
	OOCore::g_ptrRegistryRoot = pRootKey;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Registry::IRegistryKey*,IRegistryKey_OpenKey,2,((in),const string_t&,key,(in),Registry::IRegistryKey::OpenFlags_t,flags))
{
	return OOCore::g_ptrRegistryRoot->OpenSubKey(key,flags);
}
