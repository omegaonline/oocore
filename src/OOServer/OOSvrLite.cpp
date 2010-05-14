///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOSvrLite, the Omega Online Standalone plugin.
//
// OOSvrLite is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrLite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOSvrLite.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer_Lite.h"
#include "IPSLite.h"

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

using namespace Omega;
using namespace OTL;

// Our library map
BEGIN_LIBRARY_OBJECT_MAP()
END_LIBRARY_OBJECT_MAP_NO_ENTRYPOINT()

#if defined(_WIN32)

extern "C" BOOL WINAPI DllMain(HANDLE /*instance*/, DWORD /*reason*/, LPVOID /*lpreserved*/)
{
	return TRUE;
}

#endif

namespace OOBase
{
	// This is the critical failure hook
	void CriticalFailure(const char* msg)
	{
		throw ISystemException::Create(string_t(msg,false),L"Critical Failure");
	}
}

void InterProcessService::Load(const init_arg_map_t& args)
{
	m_args = args;
}

string_t InterProcessService::GetArg(const string_t& arg)
{
	const init_arg_map_t::const_iterator i = m_args.find(arg);
	if (i == m_args.end())
		return string_t();
	
	return i->second;
}

Activation::IRunningObjectTable* InterProcessService::GetRunningObjectTable()
{
	throw Remoting::IChannelClosedException::Create();
}

void InterProcessService::LaunchObjectApp(const guid_t&, const guid_t&, IObject*&)
{
	OMEGA_THROW(L"Invalid standalone function");
}

bool_t InterProcessService::HandleRequest(uint32_t timeout)
{
	return Omega::HandleRequest(timeout);
}

Remoting::IChannel* InterProcessService::OpenRemoteChannel(const string_t&)
{
	OMEGA_THROW(L"Invalid standalone function");
}

Remoting::IChannelSink* InterProcessService::OpenServerSink(const guid_t&, Remoting::IChannelSink*)
{
	throw Remoting::IChannelClosedException::Create();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(OOCore::IInterProcessService*,OOSvrLite_GetIPS,1,((in),const init_arg_map_t&,args))
{
	ObjectPtr<SingletonObjectImpl<InterProcessService> > ptrIPS = SingletonObjectImpl<InterProcessService>::CreateInstancePtr();
	ptrIPS->Load(args);
	return ptrIPS.AddRef();
}
