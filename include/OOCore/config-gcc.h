#ifndef OOCORE_CONFIG_GCC_H_INCLUDED_
#define OOCORE_CONFIG_GCC_H_INCLUDED_

#define OMEGA_MAX_DEFINES	256

#define OMEGA_FUNCNAME		__PRETTY_FUNCTION__

#include <sys/types.h>
#include <new>

#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
	do { POINTER = new CONSTRUCTOR; \
		if (POINTER == 0) { OMEGA_THROW("Out of memory."); } \
	} while (0)

#define OMEGA_HAS_INT16_T
#define OMEGA_HAS_UINT16_T
#define OMEGA_HAS_INT32_T
#define OMEGA_HAS_UINT32_T
#define OMEGA_HAS_INT64_T
#define OMEGA_HAS_UINT64_T

#define OMEGA_UNUSED_ARG(n)	(void)(n)

#define OMEGA_COMPILER_STRING_II(a,b,c,d)   a #b "." #c "." #d
#define OMEGA_COMPILER_STRING_I(a,b,c,d)    OMEGA_COMPILER_STRING_II(a,b,c,d)
#define OMEGA_COMPILER_STRING               OMEGA_COMPILER_STRING_I("mingw32-gcc-",__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__)

#endif // OOCORE_CONFIG_GCC_H_INCLUDED_
