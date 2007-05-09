#ifndef OOCORE_CONFIG_MSVC_H_INCLUDED_
#define OOCORE_CONFIG_MSVC_H_INCLUDED_

#define OMEGA_MAX_DEFINES	249

#define OMEGA_FUNCNAME		__FUNCSIG__

#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
	do { POINTER = new CONSTRUCTOR; \
		if (POINTER == 0) { OMEGA_THROW("Out of memory!"); } \
	} while (0)

#define OMEGA_IMPORT __declspec(dllimport)
#define OMEGA_EXPORT __declspec(dllexport)

#define OMEGA_UNUSED_ARG(n)	(n)

// Prevent inclusion of old winsock
#define _WINSOCKAPI_

// Warning 4127 is rubbish!
#pragma warning(disable : 4127)

#define OMEGA_COMPILER_STRING_II(a,b) a # b
#define OMEGA_COMPILER_STRING_I(a,b)  OMEGA_COMPILER_STRING_II(a,b)
#define OMEGA_COMPILER_STRING         OMEGA_COMPILER_STRING_I("MSVC ",_MSC_VER)

#endif // OOCORE_CONFIG_MSVC_H_INCLUDED_
