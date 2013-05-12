///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2012 Rick Taylor
//
// This file is part of OOSvrHost, the Omega Online user host application.
//
// OOSvrHost is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrHost is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOSvrHost.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOSERVER_HOST_H_INCLUDED_
#define OOSERVER_HOST_H_INCLUDED_

// Mingw can include pid_t etc, which we don't want
#if defined(__MINGW32__)
#define _NO_OLDNAMES
#endif

//////////////////////////////////////////////

#include <OOBase/CmdArgs.h>
#include <OOBase/Thread.h>
#include <OOBase/Logger.h>
#include <OOBase/Socket.h>
#include <OOBase/StackAllocator.h>

//////////////////////////////////////////////

#include "../oocore-config.h"

//////////////////////////////////////////////

#if defined(_WIN32) && !defined(__MINGW32__)
#if defined(WIN64_HYBRID)
	#define APPNAME "OOSvrHost64"
#else
	#define APPNAME "OOSvrHost32"
#endif
#else
	#define APPNAME "oosvrhost"
#endif

/////////////////////////////////////////////////

// Ensure we have the local Omega.h instead of the installed one...
#include "../../include/Omega/Omega.h"
#include "../../include/Omega/Remoting.h"
#include "../../include/Omega/Service.h"

#include "../../include/OTL/OTL.h"

//#include "../../include/Omega/Service.h"

#include "../OOCore/Server.h"

#define OMEGA_CREATE_INTERNAL(e)  (Omega::System::Internal::auto_iface_ptr<Omega::IInternalException>(Omega::IInternalException::Create(Omega::string_t::constant(e),__FILE__,__LINE__,OMEGA_FUNCNAME)))

namespace Host
{
	bool is_debug();

	int Surrogate();
	int ServiceStart();

	void StartService(Omega::System::IService* pService, const Omega::string_t& strName, const Omega::string_t& strPipe, Omega::Registry::IKey* pKey, const Omega::string_t& strSecret);

#if defined(_WIN32)
	int ShellEx(const OOBase::CmdArgs::results_t& args);
#endif
}

#endif // OOSERVER_HOST_H_INCLUDED_
