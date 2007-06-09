#ifndef OOCORE_CONFIG_UNIX_H_INCLUDED_
#define OOCORE_CONFIG_UNIX_H_INCLUDED_

#if defined (__GNUC__)
#include <OOCore/config-unix-gcc.h>
#else
#error Unsupported compiler!
#endif

#define OMEGA_UNIX

#define OMEGA_PLATFORM_STRING	"Unix"

#define OMEGA_EXPORT
#define OMEGA_IMPORT

#endif // OOCORE_CONFIG_UNIX_H_INCLUDED_
