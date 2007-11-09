#include "OOCore_precomp.h"

#include "./UserSession.h"
#include "./StdObjectManager.h"
#include "./Channel.h"

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

using namespace Omega;
using namespace OTL;

// Our library map
BEGIN_LIBRARY_OBJECT_MAP()
	OBJECT_MAP_ENTRY_UNNAMED(OOCore::WireProxyMarshalFactory)
	OBJECT_MAP_ENTRY_UNNAMED(OOCore::StdObjectManagerMarshalFactory)
	OBJECT_MAP_ENTRY_UNNAMED(OOCore::ChannelMarshalFactory)
	OBJECT_MAP_ENTRY_UNNAMED(OOCore::StdObjectManager)
END_LIBRARY_OBJECT_MAP()

#if defined(OMEGA_WIN32)
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason)
{
#if !defined(ACE_HAS_DLL) || (ACE_HAS_DLL != 1)
	if (reason == DLL_PROCESS_ATTACH)
	{
		// If ACE is linked statically we need to do this...
		ACE_OS::set_win32_resource_module(instance);
	}
	else if (reason == DLL_THREAD_DETACH)
	{
		// If ACE is linked statically we need to do this...
		ACE_OS::cleanup_tss(0);
	}
#endif

	return TRUE;
}
#endif

namespace OOCore
{
	static ACE_Atomic_Op<ACE_Thread_Mutex,Omega::uint32_t> s_initcount = 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,Omega_GetVersion,0,())
{
	return string_t::Format(L"Version: %hs\nPlatform: %hs\nCompiler: %hs\nACE: %hs",OMEGA_VERSION,OMEGA_PLATFORM_STRING,OMEGA_COMPILER_STRING,ACE_VERSION);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,Omega_Initialize,0,())
{
	bool bStart = false;
	if (++OOCore::s_initcount==1)
	{
		bStart = true;

		// Call ACE::init() first
		int ret = ACE::init();
		if (ret == 1)
			ret = 0;

		if (ret != 0)
		{
			--OOCore::s_initcount;
			ObjectImpl<ExceptionImpl<IException> >* pE = ObjectImpl<ExceptionImpl<IException> >::CreateInstance();
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
