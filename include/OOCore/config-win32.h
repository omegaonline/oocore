#ifndef OOCORE_CONFIG_WIN32_H_INCLUDED_
#define OOCORE_CONFIG_WIN32_H_INCLUDED_

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x400
#endif

#include <winsock2.h>

#if defined (_MSC_VER)
#include <OOCore/config-win32-msvc.h>
#elif defined (__GNUC__)
#include <OOCore/config-win32-gcc.h>
#else
#error Unsupported compiler!
#endif

// Complain if WIN32 is not already defined.
#if !defined (WIN32)
# error Please define WIN32 in your project settings.
#endif

#define OMEGA_WIN32
#if defined (__WIN64) || defined (_WIN64) || defined (WIN64)
#  define OMEGA_WIN64
#endif /* _WIN64 || WIN64 */

#define OMEGA_HAS_BUILTIN_ATOMIC_OP_4
#ifdef OMEGA_WIN64
#define OMEGA_HAS_BUILTIN_ATOMIC_OP_8
#endif

#define OMEGA_CALL __cdecl

#endif // OOCORE_CONFIG_WIN32_H_INCLUDED_
