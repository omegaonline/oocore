#ifndef OOCORE_CONFIG_MSVC_H_INCLUDED_
#define OOCORE_CONFIG_MSVC_H_INCLUDED_

/*#if (_MSC_VER >= 1400)
# include "ace/config-win32-msvc-8.h"
#elif (_MSC_VER >= 1310)
# include "ace/config-win32-msvc-7.h"
#else
# error This version of Microsoft Visual C++ not supported.
#endif*/

#define OOCORE_FUNCNAME		__PRETTY_FUNCTION__

#include <winsock2.h>
#include <sys/types.h>
#include <new>

#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
	do { POINTER = new (std::nothrow) CONSTRUCTOR; \
		if (POINTER == 0) { OMEGA_THROW("Out of memory."); } \
	} while (0)

#define OMEGA_IMPORT __declspec(dllimport)
#define OMEGA_EXPORT __declspec(dllexport)

#define interface struct

#endif // OOCORE_CONFIG_MSVC_H_INCLUDED_
