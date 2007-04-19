#ifndef OOCORE_CONFIG_GCC_H_INCLUDED_
#define OOCORE_CONFIG_GCC_H_INCLUDED_

#define OMEGA_MAX_DEFINES	256

#define OMEGA_FUNCNAME		__PRETTY_FUNCTION__

#include <winsock2.h>
#include <sys/types.h>
#include <new>

#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
	do { POINTER = new CONSTRUCTOR; \
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

#endif // OOCORE_CONFIG_GCC_H_INCLUDED_
