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

#include "UserSession.h"
#include "Activation.h"
#include "StdObjectManager.h"
#include "ApartmentImpl.h"
#include "WireProxy.h"
#include "Channel.h"
#include "Exception.h"

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

using namespace Omega;
using namespace OTL;

// Our library map
BEGIN_LIBRARY_OBJECT_MAP()
	OBJECT_MAP_ENTRY(OOCore::StdObjectManager,0)
	OBJECT_MAP_ENTRY(OOCore::ApartmentImpl,0)
	OBJECT_MAP_ENTRY(OOCore::ProxyMarshalFactory,0)
	OBJECT_MAP_ENTRY(OOCore::ChannelMarshalFactory,0)
	OBJECT_MAP_ENTRY(OOCore::CDRMessageMarshalFactory,0)
	OBJECT_MAP_ENTRY(OOCore::SystemExceptionMarshalFactoryImpl,0)
	OBJECT_MAP_ENTRY(OOCore::NoInterfaceExceptionMarshalFactoryImpl,0)
	OBJECT_MAP_ENTRY(OOCore::TimeoutExceptionMarshalFactoryImpl,0)
END_LIBRARY_OBJECT_MAP()

#if defined(_WIN32)

extern "C" BOOL WINAPI DllMain(HANDLE /*instance*/, DWORD reason, LPVOID /*lpreserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
	}
	else if (reason == DLL_THREAD_DETACH)
	{
		OOBase::TLS::ThreadExit();
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		//OOBase::Destructor::call_destructors();
	}

	return TRUE;
}
#endif

namespace OOCore
{
	static OOBase::AtomicInt<unsigned long> s_initcount = 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,Omega_GetVersion,0,())
{
#if defined(OMEGA_DEBUG)
	return string_t::Format(L"Version: %hs (Debug build)\nPlatform: %hs\nCompiler: %hs",OOCORE_VERSION,OMEGA_PLATFORM_STRING,OMEGA_COMPILER_STRING);
#else
	return string_t::Format(L"Version: %hs\nPlatform: %hs\nCompiler: %hs",OOCORE_VERSION,OMEGA_PLATFORM_STRING,OMEGA_COMPILER_STRING);
#endif
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,Omega_Initialize,0,())
{
	bool bStart = false;
	if (++OOCore::s_initcount==1)
	{
		bStart = true;

#if defined(OMEGA_DEBUG) && defined(_WIN32)
		// If this event exists, then we are being debugged
		OOBase::Win32::SmartHandle hDebugEvent(OpenEventW(EVENT_ALL_ACCESS,FALSE,L"Local\\OOCORE_DEBUG_MUTEX"));
		if (hDebugEvent)
		{
			// Wait for a bit, letting the caller attach a debugger
			WaitForSingleObject(hDebugEvent,60000);
		}
#endif
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
