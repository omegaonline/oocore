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

#if defined(HAVE_VLD_H)
#include <vld.h>
#endif

#include "UserSession.h"
#include "Activation.h"

using namespace Omega;
using namespace OTL;

#if defined(_WIN32)

extern "C" BOOL WINAPI DllMain(HANDLE /*instance*/, DWORD reason, LPVOID /*lpreserved*/)
{
	if (reason == DLL_THREAD_DETACH)
	{
		OOBase::TLS::ThreadExit();
	}
	return TRUE;
}

#endif

extern "C" OMEGA_EXPORT const char* OOCore_GetVersion()
{
#if defined(OMEGA_DEBUG)
	return OOCORE_VERSION " (Debug build)";
#else
	return OOCORE_VERSION;
#endif
}

extern "C" OMEGA_EXPORT unsigned int OOCore_GetMajorVersion()
{
	return OOCORE_MAJOR_VERSION;
}

extern "C" OMEGA_EXPORT unsigned int OOCore_GetMinorVersion()
{
	return OOCORE_MINOR_VERSION;
}

extern "C" OMEGA_EXPORT unsigned int OOCore_GetPatchVersion()
{
	return OOCORE_PATCH_VERSION;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,OOCore_Omega_Initialize,1,((in),const string_t&,args))
{
	return OOCore::UserSession::init(args);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_Omega_Uninitialize,0,())
{
	if (OOCore::HostedByOOServer())
	{
		// This is a short-cut close for use by the OOServer
		OOCore::UserSession::close_singletons();
	}
	else
	{
		OOCore::UserSession::term();
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_add_uninit_call,2,((in),Omega::Threading::DestructorCallback,pfn_dctor,(in),void*,param))
{
	return OOCore::UserSession::add_uninit_call(pfn_dctor,param);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_remove_uninit_call,2,((in),Omega::Threading::DestructorCallback,pfn_dctor,(in),void*,param))
{
	return OOCore::UserSession::remove_uninit_call(pfn_dctor,param);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_allocate,1,((in),size_t,bytes))
{
	void* p = OOBase::HeapAllocate(bytes);
	if (!p)
		OMEGA_THROW_NOMEM();
	
	return p;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_free,1,((in),void*,mem))
{
	OOBase::HeapFree(mem);
}
