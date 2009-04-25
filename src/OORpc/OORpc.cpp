///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OORpc, the Omega Online RPC library.
//
// OORpc is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OORpc is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OORpc.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OORpc.h"

#include "HttpChannelServer.h"
#include "HttpEndpoint.h"
#include "HttpMsg.h"

#ifdef OMEGA_HAVE_VLD
#include <vld.h>
#endif

using namespace Omega;
using namespace OTL;

// Our library map
BEGIN_LIBRARY_OBJECT_MAP()
	OBJECT_MAP_ENTRY(Rpc::HttpChannelServer,L"Omega.Rpc.HttpChannelServer")
	OBJECT_MAP_ENTRY(Rpc::HttpEndpoint,L"Omega.Rpc.HttpEndpoint")
	OBJECT_MAP_ENTRY(Rpc::HttpOutputMsg,0)
	OBJECT_MAP_ENTRY(Rpc::HttpOutputMsgMarshalFactory,0)
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

namespace OOBase
{
	// This is the critical failure hook
	void CriticalFailure(const char* /*msg*/)
	{
	}
}

#endif

// Install function
OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_InstallLibrary,3,((in),Omega::bool_t,bInstall,(in),Omega::bool_t,bLocal,(in),const Omega::string_t&,strSubsts))
{
	if (bLocal)
		OMEGA_THROW(L"OORpc will not install locally!");

	Rpc::HttpEndpoint::install(bInstall,strSubsts);
}
