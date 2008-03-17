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

#ifndef OOCORE_CONFIG_MSVC_H_INCLUDED_
#define OOCORE_CONFIG_MSVC_H_INCLUDED_

#ifndef _CPPUNWIND
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

#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
	do { POINTER = new CONSTRUCTOR; \
		if (POINTER == 0) { OMEGA_THROW(ENOMEM); } \
	} while (0)

#define OMEGA_IMPORT __declspec(dllimport)
#define OMEGA_EXPORT __declspec(dllexport)

#define OMEGA_UNUSED_ARG(n)	(n)

#define OMEGA_HAS_UUIDOF

// Warning 4127 is rubbish!
#pragma warning(disable : 4127)

#ifndef _DEBUG
// Optimization sometimes re-orders things causing this error
#pragma warning(disable : 4702)
#endif

#define OMEGA_COMPILER_STRING_III(n)  #n
#define OMEGA_COMPILER_STRING_II(a,b) OMEGA_COMPILER_STRING_III(a b)
#define OMEGA_COMPILER_STRING_I(a,b)  OMEGA_COMPILER_STRING_II(a,b)
#define OMEGA_COMPILER_STRING         OMEGA_COMPILER_STRING_I(MSVC,_MSC_VER)

#endif // OOCORE_CONFIG_MSVC_H_INCLUDED_
