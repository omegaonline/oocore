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
	return OOCORE_VERSION;
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
	try
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
	catch (Omega::IException* pE)
	{
		pE->Release();
	}
	catch (...)
	{
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

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::IO::IStream*,OOCore_IO_OpenStream,2,((in),const Omega::string_t&,strEndpoint,(in),Omega::IO::IAsyncStreamNotify*,pNotify))
{
	// First try to determine the protocol...
	size_t pos = strEndpoint.Find(L':');
	if (pos == string_t::npos)
		throw IInternalException::Create("No protocol specified","Omega::IO::OpenStream");

	// Look up handler in registry
	string_t strProtocol = strEndpoint.Left(pos).ToLower();

	string_t strHandler;
	ObjectPtr<Omega::Registry::IKey> ptrKey(L"\\Local User");
	if (ptrKey->IsSubKey(L"Networking\\Protocols\\" + strProtocol))
	{
		ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\" + strProtocol);
		if (ptrKey->IsValue(L"Handler"))
			strHandler = ptrKey->GetStringValue(L"Handler");
	}

	if (strHandler.IsEmpty())
	{
		ptrKey = ObjectPtr<Omega::Registry::IKey>(L"\\System");
		if (ptrKey->IsSubKey(L"Networking\\Protocols\\" + strProtocol))
		{
			ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\" + strProtocol);
			if (ptrKey->IsValue(L"Handler"))
				strHandler = ptrKey->GetStringValue(L"Handler");
		}
	}

	// Create the handler...
	ObjectPtr<Net::IProtocolHandler> ptrHandler(strHandler);

	// Open the stream...
	return ptrHandler->OpenStream(strEndpoint,pNotify);
}
