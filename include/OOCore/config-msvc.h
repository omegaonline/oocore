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

#ifndef OOCORE_CONFIG_MSVC_H_INCLUDED_
#define OOCORE_CONFIG_MSVC_H_INCLUDED_

/////////////////////////////////////////////////////////
//
// This file sets up the build environment for 
// Microsoft Visual Studio
//
/////////////////////////////////////////////////////////

#if !defined(_MSC_VER)
#error This is the MSVC build!
#elif (_MSC_VER < 1310)
#error Omega Online will not compile with a pre Visual C++ .NET 2003 compiler
#endif

#if defined(__cplusplus) && !defined(_CPPUNWIND)
#error You must enable exception handling /GX
#endif

#ifndef _MT
#error You must enable multithreaded library use /MT, /MTd, /MD or /MDd
#endif 

#if defined(_DEBUG) && !defined(OMEGA_DEBUG)
#define OMEGA_DEBUG
#endif

#define OMEGA_MAX_DEFINES 249

#define OMEGA_FUNCNAME    __FUNCSIG__

#define OMEGA_IMPORT   __declspec(dllimport)
#define OMEGA_EXPORT   __declspec(dllexport)
#define OMEGA_PRIVATE
#define OMEGA_CALL     __cdecl

#define OMEGA_UNUSED_ARG(n)	(n)

#define OMEGA_HAS_UUIDOF

// 'function' : unreferenced local function has been removed
#pragma warning(disable : 4505)

#ifndef _DEBUG
// Optimization sometimes re-orders things causing this error
#pragma warning(disable : 4702)
#endif

// Check for 64-bit builds
#if defined(_M_IA64) || defined(_M_X64)
#define OMEGA_64
#endif

#define OMEGA_COMPILER_STRING_III(n)  #n
#define OMEGA_COMPILER_STRING_II(a,b) OMEGA_COMPILER_STRING_III(a b)
#define OMEGA_COMPILER_STRING_I(a,b)  OMEGA_COMPILER_STRING_II(a,b)
#define OMEGA_COMPILER_STRING         OMEGA_COMPILER_STRING_I(MSVC,_MSC_VER)

#if defined(_WIN32_WCE)
#include "config-wince.h"
#elif defined(_WIN32)
#include "config-win32.h"
#else
#error What else can MSVC compile?
#endif

#ifdef __cplusplus
	#include <new>

#if defined(_WIN32)
	#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
		do { \
			POINTER = new (std::nothrow) CONSTRUCTOR; \
			if (!POINTER) OMEGA_THROW(ERROR_OUTOFMEMORY); \
		} while (!POINTER)
#else
	#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
		do { \
			POINTER = new (std::nothrow) CONSTRUCTOR; \
			if (!POINTER) OMEGA_THROW(ENOMEM); \
		} while (!POINTER)
#endif

#endif

#include <errno.h>
#include <assert.h>

#if !defined(EINVAL)
#define EINVAL 22
#endif

#endif // OOCORE_CONFIG_MSVC_H_INCLUDED_
