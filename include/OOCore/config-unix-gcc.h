#ifndef OOCORE_CONFIG_UNIX_GCC_H_INCLUDED_
#define OOCORE_CONFIG_UNIX_GCC_H_INCLUDED_

#include <OOCore/config-gcc.h>

#undef interface
#define interface struct

#define OMEGA_CALL __attribute__((cdecl))

#include <stdarg.h>

#endif // OOCORE_CONFIG_UNIX_GCC_H_INCLUDED_
