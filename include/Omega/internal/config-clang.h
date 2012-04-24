///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
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

#ifndef OOCORE_CONFIG_CLANG_H_INCLUDED_
#define OOCORE_CONFIG_CLANG_H_INCLUDED_

// clang is so close to gcc, that we include config-gcc.h and then override what we want
#include "config-gcc.h"

#undef OMEGA_COMPILER_STRING
#define OMEGA_COMPILER_STRING   __VERSION__ ": version "__clang_version__

#endif // OOCORE_CONFIG_CLANG_H_INCLUDED_
