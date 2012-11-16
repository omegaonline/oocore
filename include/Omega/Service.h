///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
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

#ifndef OMEGA_SERVICE_H_INCLUDED_
#define OMEGA_SERVICE_H_INCLUDED_

#include "./Registry.h"

#if defined(_WIN32)
#if defined(__MINGW32__) && defined(_WINSOCKAPI_)
#undef _WINSOCKAPI_
#endif
#include <winsock2.h>
#endif

namespace Omega
{
	namespace System
	{
		interface IService : public IObject
		{
#if defined(_WIN32)
			typedef SOCKET socket_t;
#else
			typedef int socket_t;
#endif
			typedef std::map<string_t,socket_t,std::less<string_t>,System::STLAllocator<string_t> > socket_map_t;

			virtual void Run(const string_t& strName, Registry::IKey* pKey, socket_map_t& socket_map) = 0;
			virtual void Stop() = 0;
		};

		interface IServiceController : public IObject
		{
			typedef std::set<string_t,std::less<string_t>,System::STLAllocator<string_t> > service_set_t;

			virtual void StartService(const string_t& strName) = 0;
			virtual void StopService(const string_t& strName) = 0;
			virtual bool_t IsServiceRunning(const string_t& strName) = 0;
			virtual service_set_t GetRunningServices() = 0;
		};

		// {D2A10F8C-ECD1-F698-7105-48247D50DB1B}
		OOCORE_DECLARE_OID(OID_ServiceController);
	}
}

OMEGA_DEFINE_INTERFACE
(
	Omega::System, IService, "{C5BA215C-C59C-4471-96F8-9169F122150A}",

	OMEGA_METHOD_VOID(Run,3,((in),const string_t&,strName,(in),Registry::IKey*,pKey,(in_out),System::IService::socket_map_t&,socket_map))
	OMEGA_METHOD_VOID(Stop,0,())
)

OMEGA_DEFINE_INTERFACE
(
	Omega::System, IServiceController, "{74EB6BA9-497C-D466-E591-9896F6BB8E1B}",

	OMEGA_METHOD_VOID(StartService,1,((in),const string_t&,strName))
	OMEGA_METHOD_VOID(StopService,1,((in),const string_t&,strName))
	OMEGA_METHOD(bool_t,IsServiceRunning,1,((in),const string_t&,strName))
	OMEGA_METHOD(System::IServiceController::service_set_t,GetRunningServices,0,())
)

#endif // OMEGA_SERVICE_H_INCLUDED_
