///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOCORE_CONFIG_GCC_H_INCLUDED_
#define OOCORE_CONFIG_GCC_H_INCLUDED_

#ifdef __cplusplus
	#include <new>
	#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
		do { POINTER = new (std::nothrow) CONSTRUCTOR; \
			if (POINTER == 0) { OMEGA_THROW(L"Out of memory."); } \
		} while (0)
#endif

#if !defined(HAVE_STDINT_H)
#define HAVE_STDINT_H 1
#endif

#define OMEGA_UNUSED_ARG(n)    (void)(n)

#define OMEGA_COMPILER_STRING  "gcc " __VERSION__

#undef interface
#define interface struct

#define OMEGA_FUNCNAME		__PRETTY_FUNCTION__

/* stop lots of attributes warnings */
#ifndef __x86_64__
    #define OMEGA_CALL   __attribute__((__cdecl__))
#else
    #define OMEGA_CALL
#endif /* ndef __x86_64__ */

#ifdef __LP64__
    #define OMEGA_64
#endif

#if defined(__ELF__)
    #if __GNUC__ >= 4
        #define OMEGA_EXPORT  __attribute__((visibility("default")))
        #define OMEGA_IMPORT  __attribute__((visibility("default")))
        #define OMEGA_PRIVATE __attribute__((visibility("hidden")))
    #else
        #error Need a version script!
    #endif
#elif defined(__MINGW32__) || defined(__CYGWIN__)
    #define OMEGA_IMPORT  __attribute__((dllimport))
    #define OMEGA_EXPORT  __attribute__((dllexport))
    #define OMEGA_PRIVATE
#else
    #error No idea how to control export for this output!
#endif

#if defined(__MINGW32__)
	// We assume win32 for MinGW
    #include <OOCore/config-win32.h>
#elif defined(__unix__)
	// We assume we are some kind of unix
    #define OMEGA_PLATFORM_STRING "Unix"
#else
	#error What platform is this?
#endif

#include <errno.h>
#include <stdarg.h>

#if defined(HAVE_ASSERT_H)
#include <assert.h>
#else
#define assert(x) ((void)0)
#endif

#endif // OOCORE_CONFIG_GCC_H_INCLUDED_
