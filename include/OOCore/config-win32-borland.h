#ifndef OOCORE_CONFIG_BORLAND_H_INCLUDED_
#define OOCORE_CONFIG_BORLAND_H_INCLUDED_

#define OMEGA_MAX_DEFINES     124

#define OMEGA_FUNCNAME		__FUNC__

#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
	do { POINTER = new (std::nothrow) CONSTRUCTOR; \
		if (POINTER == 0) { OMEGA_THROW("Out of memory."); } \
	} while (0)

#define OMEGA_IMPORT __declspec(dllimport)
#define OMEGA_EXPORT __declspec(dllexport)

#define OMEGA_HAS_INT16_T
#define OMEGA_HAS_UINT16_T
#define OMEGA_HAS_INT32_T
#define OMEGA_HAS_UINT32_T
#define OMEGA_HAS_INT64_T
#define OMEGA_HAS_UINT64_T

#define OMEGA_UNUSED_ARG(n)	(n)

// Dont know how to do this better!
#pragma comment (lib, "ACE_bsd.lib")
#pragma comment (lib, "ws2_32.lib")
#pragma comment (lib, "shlwapi.lib")

// Turn of pointless warnings!
#pragma warn -8022
#pragma warn -8058
#pragma warn -8074

#endif // OOCORE_CONFIG_BORLAND_H_INCLUDED_
