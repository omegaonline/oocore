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

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

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

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,OOCore_Omega_Initialize,0,())
{
	return OOCore::UserSession::init(false,std::map<Omega::string_t,Omega::string_t>());
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,OOCore_Omega_InitStandalone,1,((in),const Omega::init_arg_map_t&,args))
{
	return OOCore::UserSession::init(true,args);
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

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_add_uninit_call,2,((in),void*,pfn_dctor,(in),void*,param))
{
	return OOCore::UserSession::add_uninit_call((void (OMEGA_CALL*)(void*))pfn_dctor,param);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_remove_uninit_call,2,((in),void*,pfn_dctor,(in),void*,param))
{
	return OOCore::UserSession::remove_uninit_call((void (OMEGA_CALL*)(void*))pfn_dctor,param);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_allocate,4,((in),size_t,len,(in),int,flags,(in),const char*,file,(in),unsigned int,line))
{
	OMEGA_UNUSED_ARG(file);
	OMEGA_UNUSED_ARG(line);

	/*std::ostringstream os;
	os << "Alloc(" << flags << ") " << len << " " << file << " " << line << std::endl;
	OutputDebugString(os.str().c_str());*/

	if (flags == 2)
	{
#if defined(_MSC_VER)
		void* sp = _malloca(len);
		if (!sp)
			OMEGA_THROW(ENOMEM);
		return sp;
#else
		flags = 1;
#endif
	}
	
	void* p = ::malloc(len);
	if (!p)
		OMEGA_THROW(ENOMEM);
	
	return p;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_free,2,((in),void*,mem,(in),int,flags))
{
	/*std::ostringstream os;
	os << "Free:  " << mem << std::endl;
	OutputDebugString(os.str().c_str());*/

	if (flags == 2)
	{
#if defined(_MSC_VER)
		_freea(mem);
		return;
#else
		flags = 1;
#endif
	}
	
	::free(mem);
}
