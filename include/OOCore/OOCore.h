#ifndef OOCORE_H_INCLUDED_
#define OOCORE_H_INCLUDED_

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

#include <OOCore/config.h>

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
