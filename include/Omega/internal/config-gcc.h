///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
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

#ifndef OMEGA_CONFIG_GCC_H_INCLUDED_
#define OMEGA_CONFIG_GCC_H_INCLUDED_

#if !defined(HAVE_STDINT_H)
#define HAVE_STDINT_H 1
#endif

#define OMEGA_UNUSED_ARG(n)    (void)(n)
#define OMEGA_COMPILER_STRING  "gcc " __VERSION__
#define OMEGA_FUNCNAME         __PRETTY_FUNCTION__

/* stop lots of attributes warnings */
#if !defined(__x86_64__)
	#define OMEGA_CALL   __attribute__((__cdecl__))
#else
	#define OMEGA_CALL
#endif /* ndef __x86_64__ */

#if defined(__LP64__)
	#define OMEGA_64
#endif

#if !defined(__EXCEPTIONS) || (__EXCEPTIONS != 1)
	#error Dont use -fno-exceptions!
#endif

#if defined(__ELF__)
	#define OMEGA_IMPORT

	#if (__GNUC__ >= 4)
		#define OMEGA_EXPORT  __attribute__((visibility("default")))
	#elif !defined(OMEGA_MODULE_PRIVATE_NAME)
		#error You must define OMEGA_MODULE_PRIVATE_NAME to control symbol visibility
	#else
		#define OMEGA_EXPORT
	#endif
#elif defined(_WIN32)
	#define OMEGA_IMPORT  __attribute__((dllimport))
	#define OMEGA_EXPORT  __attribute__((dllexport))
#else
	#error No idea how to control symbol visibility for this compiler/linker/output format!
#endif

#if !defined(OMEGA_MODULE_PRIVATE_NAME)
	#define OMEGA_MODULE_PRIVATE_NAME
#endif

#if defined(_WIN32)
	#include "./config-win32.h"
#endif

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>

#endif // OMEGA_CONFIG_GCC_H_INCLUDED_
