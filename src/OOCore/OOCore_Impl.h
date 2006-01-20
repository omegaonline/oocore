//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_OOCORE_IMPL_H_INCLUDED_
#define OOCORE_OOCORE_IMPL_H_INCLUDED_

#include "./OOCore_Util.h"

// This is a shoddy fixup for compilers with broken explicit template specialisation
/*#if (__GNUC__) && (__GNUC__ <= 3)
	#define EXPLICIT_TEMPLATE(m,t)	template m<t>
#else
	#define EXPLICIT_TEMPLATE(m,t)	m<t>
#endif*/

namespace OOCore
{
namespace Impl
{
	extern bool g_IsServer;

#ifdef ACE_WIN32
	extern HINSTANCE g_hInstance;
#endif

};
};

#endif // OOCORE_OOCORE_IMPL_H_INCLUDED_
