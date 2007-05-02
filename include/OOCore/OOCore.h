#ifndef OOCORE_H_INCLUDED_
#define OOCORE_H_INCLUDED_

//////////////////////////////////////////////
// Set up the export macros for OOCORE
#if defined(OOCORE_BUILD_DLL)

#define OOCORE_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_LOCAL_FUNCTION_VOID(name,param_count,params)

#define OOCORE_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_LOCAL_FUNCTION(ret_type,name,param_count,params)

#else

#define OOCORE_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_EXPORTED_FUNCTION_VOID(name,param_count,params)

#define OOCORE_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_EXPORTED_FUNCTION(ret_type,name,param_count,params)

#endif

//////////////////////////////////////////////

#include <OOCore/config.h>

//////////////////////////////////////////////
// Include STL components
#if (defined(_MSC_VER) && _MSC_VER>=1300)
// Some stl functions have warnings
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

#include <list>
#include <map>
#include <vector>

#if (defined(_MSC_VER) && _MSC_VER>=1300)
// Some stl functions have warnings
#pragma warning(pop)
#endif
// End of STL includes
//////////////////////////////////////////////

#include <OOCore/OOCore_types.h>
#include <OOCore/OOCore_base.h>
#include <OOCore/OOCore_export.h>
#include <OOCore/OOCore_rtti.h>
#include <OOCore/OOCore_wire.h>
#include <OOCore/OOCore_ifaces.h>

namespace Omega
{
	inline IException* Initialize();
	inline void Uninitialize();
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

#include <OOCore/OOCore_types.inl>
#include <OOCore/OOCore_rtti.inl>

#if defined(ACE_NLOGGING)
#error You must not define ACE_NLOGGING, cos we use it!
#endif

#endif // OOCORE_H_INCLUDED_
