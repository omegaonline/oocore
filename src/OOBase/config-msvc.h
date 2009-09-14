///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOBase, the Omega Online Base library.
//
// OOBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOBASE_CONFIG_MSVC_H_INCLUDED_
#define OOBASE_CONFIG_MSVC_H_INCLUDED_

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
#elif (_MSC_VER >= 1400)
// VS-2005 and later
#define _CRT_NONSTDC_NO_WARNINGS
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

#if defined(_WIN32_WCE)
	#include "config-wince.h"
#elif defined(_WIN32)
	#include "config-win32.h"
#else
	#error What else can MSVC compile?
#endif

#define HAVE_NEW_NOTHROW 1
#define HAVE_ASSERT_H 1
#define HAVE_STRING_H 1

#endif // OOBASE_CONFIG_MSVC_H_INCLUDED_
