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

#ifndef OOCORE_CONFIG_UNIX_GCC_H_INCLUDED_
#define OOCORE_CONFIG_UNIX_GCC_H_INCLUDED_

#include <OOCore/config-gcc.h>

#undef interface
#define interface struct

/* stop lots of attributes warnings */
#ifndef __x86_64__
#define OMEGA_CALL   __attribute__((cdecl))
#else
#define OMEGA_CALL
#endif /* ndef __x86_64__ */

#ifdef __LP64__
#define OMEGA_64
#endif

#define OMEGA_EXPORT  __attribute__((visibility("default")))
#define OMEGA_IMPORT  __attribute__((visibility("default")))
#define OMEGA_PRIVATE __attribute__((visibility("hidden")))

#include <stdarg.h>

#endif // OOCORE_CONFIG_UNIX_GCC_H_INCLUDED_
