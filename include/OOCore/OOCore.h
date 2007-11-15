///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1999 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
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

#ifndef OOCORE_H_INCLUDED_
#define OOCORE_H_INCLUDED_

//////////////////////////////////////////////
#include <OOCore/config-guess.h>

//////////////////////////////////////////////
// Set up the export macros for OOCORE
#if defined(OOCORE_BUILD_LIBRARY)

#define OOCORE_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_LOCAL_FUNCTION_VOID(name,param_count,params)

#define OOCORE_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_LOCAL_FUNCTION(ret_type,name,param_count,params)

#define OOCORE_DECLARE_OID(n) \
	OMEGA_EXPORT_OID(n)

#else

#define OOCORE_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_EXPORTED_FUNCTION_VOID(name,param_count,params)

#define OOCORE_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_EXPORTED_FUNCTION(ret_type,name,param_count,params)

#define OOCORE_DECLARE_OID(n) \
	OMEGA_IMPORT_OID(n)

#endif

//////////////////////////////////////////////
// Include STL components

#include <list>
#include <map>
#include <vector>

// End of STL includes
//////////////////////////////////////////////

#include <OOCore/Version.h>
#include <OOCore/Types.h>
#include <OOCore/Threading.h>
#include <OOCore/Base.h>
#include <OOCore/Export.h>
#include <OOCore/Rtti.h>
#include <OOCore/Wire.h>
#include <OOCore/Interfaces.h>

namespace Omega
{
	inline IException* Initialize();
	inline void Uninitialize();

	namespace System
	{
		inline string_t GetVersion();
	}
}

OOCORE_EXPORTED_FUNCTION(Omega::string_t,Omega_GetVersion,0,());
Omega::string_t Omega::System::GetVersion()
{
	return Omega_GetVersion();
}

OOCORE_EXPORTED_FUNCTION(Omega::IException*,Omega_Initialize,0,());
Omega::IException* Omega::Initialize()
{
	return Omega_Initialize();
}

OOCORE_EXPORTED_FUNCTION_VOID(Omega_Uninitialize,0,());
void Omega::Uninitialize()
{
	Omega_Uninitialize();
}

#include <OOCore/Types.inl>
#include <OOCore/Threading.inl>
#include <OOCore/Rtti.inl>

#if defined(ACE_NLOGGING)
#error You must not define ACE_NLOGGING, cos we use it!
#endif

#endif // OOCORE_H_INCLUDED_
