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

#ifndef OOCORE_MSVC_H_INCLUDED_
#define OOCORE_MSVC_H_INCLUDED_

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

#if defined(_WIN32) || defined(_WIN32_WCE)
	/* Define to 1 if you have the <windows.h> header file. */
	#define HAVE_WINDOWS_H 1
#else
	#error What else can MSVC compile?
#endif

/* Define to 1 if you have the `_wcstoi64' function. */
#define HAVE__WCSTOI64 1

/* Define to 1 if you have the `_wcstoui64' function. */
#define HAVE__WCSTOUI64 1

#endif // OOCORE_MSVC_H_INCLUDED_
