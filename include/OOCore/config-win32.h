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

#ifndef OOCORE_CONFIG_WIN32_H_INCLUDED_
#define OOCORE_CONFIG_WIN32_H_INCLUDED_

// Prevent inclusion of old winsock
#define _WINSOCKAPI_

// Reduce the amount of windows we include
#define WIN32_LEAN_AND_MEAN
#define STRICT

// We support Vista API's
#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0600
#elif _WIN32_WINNT < 0x0500
#error OOCore requires _WIN32_WINNT >= 0x0500!
#endif

// We require IE 5 or later
#if !defined(_WIN32_IE)
#define _WIN32_IE 0x0500
#elif _WIN32_IE < 0x0500
#error OOCore requires _WIN32_IE >= 0x0500!
#endif

#include <windows.h>

// Check for obsolete windows versions
#if defined(_WIN32_WINDOWS)
#error You cannot build Omega Online for Windows 95/98/Me!
#endif

#if !defined(WINVER)
#error No WINVER?!?
#elif (WINVER < 0x0500)
#if defined(__MINGW32__)
// MinGW gets WINVER wrong...
#undef WINVER
#define WINVER 0x0500
#else
#error OOCore requires WINVER >= 0x0500!
#endif
#endif

#if !defined(_WIN32)
#error No _WIN32?!?
#endif

#if defined(_WIN64)
#define OMEGA_PLATFORM_STRING "Win64"
#else
#define OMEGA_PLATFORM_STRING "Win32"
#endif

#endif // OOCORE_CONFIG_WIN32_H_INCLUDED_
