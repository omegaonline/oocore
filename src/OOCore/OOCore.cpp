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

using namespace Omega;
using namespace OTL;

#if defined(_MSC_VER)
// Shutup VS leak
extern "C" int _setenvp() { return 0; }
#endif

#if defined(_WIN32)
extern "C" BOOL WINAPI DllMain(HANDLE /*instance*/, DWORD reason, LPVOID /*lpreserved*/)
{
	if (reason == DLL_THREAD_DETACH)
		OOBase::TLS::ThreadExit();

	return TRUE;
}
#endif

extern "C" OMEGA_EXPORT const char* OOCore_GetVersion()
{
#if !defined(NDEBUG)
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

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,OOCore_Omega_Initialize,1,((in),Omega::uint32_t,version))
{
	// Check the versions are correct
	if (version > ((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16)))
		return Omega::IInternalException::Create(OOCore::get_text("This component requires a later version of OOCore"),"Omega::Initialize");

	return OOCore::UserSession::init();
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_Omega_Uninitialize,0,())
{
	if (OOCore::HostedByOOServer())
	{
		// This is a short-cut close for use by the OOServer

		// Unregister built-ins
		OOCore::UnregisterObjects();

		// Close all singletons
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
	void* p = OOBase::HeapAllocator::allocate(bytes);
	if (!p)
		OMEGA_THROW(ERROR_OUTOFMEMORY);

	return p;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_free,1,((in),void*,mem))
{
	OOBase::HeapAllocator::free(mem);
}

const OOCore::throwing_t OOCore::throwing = {0};

void* operator new(size_t size, const OOCore::throwing_t&)
{
	void* p = ::operator new(size,std::nothrow);
	if (!p)
		OMEGA_THROW(ERROR_OUTOFMEMORY);

	return p;
}

void* operator new[](size_t size, const OOCore::throwing_t&)
{
	void* p = ::operator new [] (size,std::nothrow);
	if (!p)
		OMEGA_THROW(ERROR_OUTOFMEMORY);

	return p;
}

void operator delete(void* p, const OOCore::throwing_t&)
{
	::operator delete(p);
}

void operator delete[](void* p, const OOCore::throwing_t&)
{
	::operator delete[](p);
}

// {D2A10F8C-ECD1-F698-7105-48247D50DB1B}
OMEGA_DEFINE_OID(System,OID_ServiceController,"{D2A10F8C-ECD1-F698-7105-48247D50DB1B}");
