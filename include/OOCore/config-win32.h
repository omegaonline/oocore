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

#ifndef OOCORE_CONFIG_WIN32_H_INCLUDED_
#define OOCORE_CONFIG_WIN32_H_INCLUDED_

// Check WIN32 is already defined.
#if !defined (WIN32)
#if defined(__WIN32) || defined (_WIN32) || defined(__WIN32__)
#define WIN32
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

// Prevent inclusion of old winsock
#define _WINSOCKAPI_

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
#define OMEGA_WIN64
#endif /* _WIN64 || WIN64 */

#define OMEGA_CALL __cdecl

#if defined(OMEGA_WIN64)
#define OMEGA_PLATFORM_STRING	"Win64"
#else
#define OMEGA_PLATFORM_STRING	"Win32"
#endif

#define OMEGA_HAS_ATOMIC_OP
#define OMEGA_ATOMIC_OP_EXCHANGE(p,q) \
	InterlockedExchange((LPLONG)(p),(LONG)(q))

#define OMEGA_ATOMIC_OP_INCREMENT(p) \
	InterlockedIncrement((LPLONG)p)

#define OMEGA_ATOMIC_OP_DECREMENT(p) \
	InterlockedDecrement((LPLONG)p)

#endif // OOCORE_CONFIG_WIN32_H_INCLUDED_
