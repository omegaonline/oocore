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

#ifndef OOCORE_CONFIG_BORLAND_H_INCLUDED_
#define OOCORE_CONFIG_BORLAND_H_INCLUDED_

#error Borland C++ currently doesn't work - please have a go at fixing it?

#define OMEGA_MAX_DEFINES     124

#define OMEGA_FUNCNAME        __FUNC__

#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
	do { POINTER = new (std::nothrow) CONSTRUCTOR; \
		if (POINTER == 0) { OMEGA_THROW("Out of memory."); } \
	} while (0)

// Change this one day
#define OMEGA_NEW_THREAD_LOCAL(POINTER,CONSTRUCTOR) OMEGA_NEW(POINTER,CONSTRUCTOR)

#define OMEGA_IMPORT __declspec(dllimport)
#define OMEGA_EXPORT __declspec(dllexport)

#if !defined(OMEGA_MODULE_PRIVATE_NAME)
	#define OMEGA_MODULE_PRIVATE_NAME
#endif

#define OMEGA_HAS_INT16_T
#define OMEGA_HAS_UINT16_T
#define OMEGA_HAS_INT32_T
#define OMEGA_HAS_UINT32_T
#define OMEGA_HAS_INT64_T
#define OMEGA_HAS_UINT64_T

#define OMEGA_UNUSED_ARG(n) (n)

// Dont know how to do this better!
#pragma comment (lib, "ACE_bsd.lib")
#pragma comment (lib, "ws2_32.lib")
#pragma comment (lib, "shlwapi.lib")

// Turn of pointless warnings!
#pragma warn -8022
#pragma warn -8058
#pragma warn -8074

#endif // OOCORE_CONFIG_BORLAND_H_INCLUDED_
