#ifndef OOCORE_CONFIG_MSVC_H_INCLUDED_
#define OOCORE_CONFIG_MSVC_H_INCLUDED_

#ifndef _CPPUNWIND
#error You must enable exception handling /GX
#endif

#ifndef _MT
#error You must enable multithreaded library use /MT, /MTd, /MD or /MDd
#endif 

#if defined(_DEBUG) && !defined(OMEGA_DEBUG)
#define OMEGA_DEBUG
#endif

#define OMEGA_MAX_DEFINES	249

#define OMEGA_FUNCNAME		__FUNCSIG__

#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
	do { POINTER = new CONSTRUCTOR; \
		if (POINTER == 0) { OMEGA_THROW(L"Out of memory!"); } \
	} while (0)

#define OMEGA_IMPORT __declspec(dllimport)
#define OMEGA_EXPORT __declspec(dllexport)

#define OMEGA_UNUSED_ARG(n)	(n)

#define OMEGA_HAS_UUIDOF

// Prevent inclusion of old winsock
#define _WINSOCKAPI_

// Warning 4127 is rubbish!
#pragma warning(disable : 4127)

#ifndef _DEBUG
// Optimization sometimes re-orders things causing this error
#pragma warning(disable : 4702)
#endif

#define OMEGA_COMPILER_STRING_III(n)  #n
#define OMEGA_COMPILER_STRING_II(a,b) OMEGA_COMPILER_STRING_III(a b)
#define OMEGA_COMPILER_STRING_I(a,b)  OMEGA_COMPILER_STRING_II(a,b)
#define OMEGA_COMPILER_STRING         OMEGA_COMPILER_STRING_I(MSVC,_MSC_VER)

#endif // OOCORE_CONFIG_MSVC_H_INCLUDED_
