///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
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

#ifndef OOCORE_CONFIG_H_INCLUDED_
#define OOCORE_CONFIG_H_INCLUDED_

#if defined(_MSC_VER)
	#include "../oocore-msvc.h"
#else
	// Autoconf
	#include <oocore-autoconf.h>
#endif

////////////////////////////////////////
// Try to work out what's going on with MS Windows
#if defined(HAVE_WINDOWS_H)
	#if !defined(_WIN32)
	#error No _WIN32 ?!?
	#endif

	// Prevent inclusion of old winsock
	#define _WINSOCKAPI_

	// Reduce the amount of windows we include
	#define WIN32_LEAN_AND_MEAN
	#define STRICT

	// We support Vista API's
	#if !defined(_WIN32_WINNT)
	#define _WIN32_WINNT 0x0600
	#elif _WIN32_WINNT < 0x0500
	#error OOBase requires _WIN32_WINNT >= 0x0500!
	#endif

	// We require IE 5 or later
	#if !defined(_WIN32_IE)
	#define _WIN32_IE 0x0500
	#elif _WIN32_IE < 0x0500
	#error OOBase requires _WIN32_IE >= 0x0500!
	#endif

	#include <windows.h>

	#if !defined(WINVER)
	#error No WINVER?!?
	#elif (WINVER < 0x0500)
	#if defined(__MINGW32__)
	// MinGW gets WINVER wrong...
	#undef WINVER
	#define WINVER 0x0500
	#else
	#error OOBase requires WINVER >= 0x0500!
	#endif
	#endif

	// Check for obsolete windows versions
	#if defined(_WIN32_WINDOWS)
	#error You cannot build Omega Online for Windows 95/98/Me!
	#endif

	// Remove the unistd include - we are windows
	#if defined(HAVE_UNISTD_H)
	#undef HAVE_UNISTD_H
	#endif

#endif // HAVE_WINDOWS_H

////////////////////////////////////////
// Bring in POSIX if possible
#if defined(HAVE_UNISTD_H)
	#include <unistd.h>

	// check for POSIX.1 IEEE 1003.1
	#if !defined(_POSIX_VERSION)
	#error <unistd.h> is not POSIX compliant?
	#endif

#endif

#endif // OOCORE_CONFIG_H_INCLUDED_
