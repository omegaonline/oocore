#ifndef OOCORE_CONFIG_MSVC_H_INCLUDED_
#define OOCORE_CONFIG_MSVC_H_INCLUDED_

/*#if (_MSC_VER >= 1400)
# include "ace/config-win32-msvc-8.h"
#elif (_MSC_VER >= 1310)
# include "ace/config-win32-msvc-7.h"
#else
# error This version of Microsoft Visual C++ not supported.
#endif*/

#define OMEGA_MAX_DEFINES	249

#define OMEGA_FUNCNAME		__FUNCSIG__

#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
	do { POINTER = new CONSTRUCTOR; \
		if (POINTER == 0) { OMEGA_THROW("Out of memory."); } \
	} while (0)

#define OMEGA_IMPORT __declspec(dllimport)
#define OMEGA_EXPORT __declspec(dllexport)

#define OMEGA_UNUSED_ARG(n)	(n)

// Prevent inclusion of old winsock
#define _WINSOCKAPI_

// Warning 4127 is rubbish!
#pragma warning(disable : 4127)

#endif // OOCORE_CONFIG_MSVC_H_INCLUDED_
