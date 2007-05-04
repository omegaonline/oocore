#ifndef OOCORE_CONFIG_WIN32_H_INCLUDED_
#define OOCORE_CONFIG_WIN32_H_INCLUDED_

// Complain if WIN32 is not already defined.
#if !defined (WIN32)
#if defined(__WIN32) || defined (_WIN32) || defined(__WIN32__)
#define WIN32
#else
#error Please define WIN32 in your project settings.
#endif
#endif

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0500
#elif _WIN32_WINNT < 0x0500
#error OOCore requires _WIN32_WINNT >= 0x0500!
#endif

#if !defined(_WIN32_IE)
#define _WIN32_IE 0x0500
#elif _WIN32_IE < 0x0500
#error OOCore requires _WIN32_IE >= 0x0500!
#endif

#if defined (_MSC_VER)
#include <OOCore/config-win32-msvc.h>
#elif defined (__GNUC__)
#include <OOCore/config-win32-gcc.h>
#elif defined (__BORLANDC__)
#include <OOCore/config-win32-borland.h>
#else
#error Unsupported compiler!
#endif

#define OMEGA_WIN32
#if defined (__WIN64) || defined (_WIN64) || defined (WIN64)
#  define OMEGA_WIN64
#endif /* _WIN64 || WIN64 */

#define OMEGA_HAS_BUILTIN_ATOMIC_OP_4
#if defined(OMEGA_WIN64)
#define OMEGA_HAS_BUILTIN_ATOMIC_OP_8
#endif

#define OMEGA_CALL __cdecl

#include <shlobj.h>
#include <shlwapi.h>

#if defined(OMEGA_WIN64)
#define OMEGA_PLATFORM_STRING	"Win64"
#else
#define OMEGA_PLATFORM_STRING	"Win32"
#endif

#endif // OOCORE_CONFIG_WIN32_H_INCLUDED_

