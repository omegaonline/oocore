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

#include <functional>

namespace std
{
template<>
struct less< OOCore::ProxyStubManager::cookie_t > : public binary_function<OOCore::ProxyStubManager::cookie_t, OOCore::ProxyStubManager::cookie_t, bool> 
{
	bool operator()(const OOCore::ProxyStubManager::cookie_t& _Left, const OOCore::ProxyStubManager::cookie_t& _Right) const
	{
		return (_Left.slot_generation() <= _Right.slot_generation() &&
				_Left.slot_index() < _Right.slot_index());
	}
};
};

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
