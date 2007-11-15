///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1999 Rick Taylor
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

#define OMEGA_FUNCNAME		__PRETTY_FUNCTION__

#include <sys/types.h>
#include <new>

#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
	do { POINTER = new CONSTRUCTOR; \
		if (POINTER == 0) { OMEGA_THROW(L"Out of memory."); } \
	} while (0)

#define OMEGA_HAS_INT16_T
#define OMEGA_HAS_UINT16_T
#define OMEGA_HAS_INT32_T
#define OMEGA_HAS_UINT32_T
#define OMEGA_HAS_INT64_T
#define OMEGA_HAS_UINT64_T

#define OMEGA_UNUSED_ARG(n)	(void)(n)

#define OMEGA_COMPILER_STRING           "gcc " __VERSION__

#endif // OOCORE_CONFIG_GCC_H_INCLUDED_
