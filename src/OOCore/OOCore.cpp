///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOCore_precomp.h"

#include "./UserSession.h"
#include "./Activation.h"
#include "./StdObjectManager.h"
#include "./ApartmentImpl.h"
#include "./WireProxy.h"
#include "./Channel.h"
#include "./Exception.h"
#include "./HttpImpl.h"

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

using namespace Omega;
using namespace OTL;

// Our library map
BEGIN_LIBRARY_OBJECT_MAP()
	OBJECT_MAP_ENTRY_UNNAMED(OOCore::StdObjectManager)
	OBJECT_MAP_ENTRY_UNNAMED(OOCore::ApartmentImpl)
	OBJECT_MAP_ENTRY_UNNAMED(OOCore::ProxyMarshalFactory)
	OBJECT_MAP_ENTRY_UNNAMED(OOCore::ChannelMarshalFactory)
	OBJECT_MAP_ENTRY_UNNAMED(OOCore::OutputCDRMarshalFactory)
	OBJECT_MAP_ENTRY_UNNAMED(OOCore::SystemExceptionMarshalFactoryImpl)
	OBJECT_MAP_ENTRY_UNNAMED(OOCore::NoInterfaceExceptionMarshalFactoryImpl)
	OBJECT_MAP_ENTRY(OOCore::HttpRequest,L"Omega.Http.Request")
END_LIBRARY_OBJECT_MAP()

#if defined(OMEGA_WIN32)
extern "C" BOOL WINAPI DllMain(HANDLE instance, DWORD reason, LPVOID /*lpreserved*/)
{
#if !defined(ACE_HAS_DLL) || (ACE_HAS_DLL != 1)
	if (reason == DLL_PROCESS_ATTACH)
	{
		// Call ACE::init() first
		ACE::init();

		// If ACE is linked statically we need to do this...
		ACE_OS::set_win32_resource_module((HINSTANCE)instance);
	}
	else if (reason == DLL_THREAD_DETACH)
	{
		ACE_OS::cleanup_tss(0);
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		ACE::fini();
	}
#else
	OMEGA_UNUSED_ARG(instance);
	OMEGA_UNUSED_ARG(reason);
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
#if defined(OMEGA_DEBUG)
	return string_t::Format(L"Version: %hs (Debug build)\nPlatform: %hs\nCompiler: %hs\nACE: %hs",OMEGA_VERSION,OMEGA_PLATFORM_STRING,OMEGA_COMPILER_STRING,ACE_VERSION);
#else
	return string_t::Format(L"Version: %hs\nPlatform: %hs\nCompiler: %hs\nACE: %hs",OMEGA_VERSION,OMEGA_PLATFORM_STRING,OMEGA_COMPILER_STRING,ACE_VERSION);
#endif
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,Omega_Initialize,0,())
{
	bool bStart = false;
	if (++OOCore::s_initcount==1)
	{
		bStart = true;

#if defined(OMEGA_DEBUG) && defined(OMEGA_WIN32)
		// If this event exists, then we are being debugged
		HANDLE hDebugEvent = OpenEventW(EVENT_ALL_ACCESS,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX");
		if (hDebugEvent)
		{
			// Wait for a bit, letting the caller attach a debugger
			WaitForSingleObject(hDebugEvent,60000);
			CloseHandle(hDebugEvent);
		}
#endif
		// Turn off all ACE logging
		ACE_Log_Msg::instance()->priority_mask(0,ACE_Log_Msg::PROCESS);
	}

	if (bStart)
	{
		IException* pE = OOCore::UserSession::init();
		if (pE)
			return pE;
	}

	return 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_Uninitialize,0,())
{
	if (OOCore::HostedByOOServer())
	{
		// This is a short-cut close for use by the OOServer
		OOCore::SERVICE_MANAGER::close();
	}
	else if (--OOCore::s_initcount==0)
	{
		OOCore::UserSession::term();
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::IO::IStream*,Omega_IO_OpenStream,2,((in),const Omega::string_t&,strEndpoint,(in),Omega::IO::IAsyncStreamNotify*,pNotify))
{
	// Ask the IPS to open the stream...
	return OOCore::GetInterProcessService()->OpenStream(strEndpoint,pNotify);
}
