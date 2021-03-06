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

#ifndef OMEGA_CONFIG_GUESS_H_INCLUDED_
#define OMEGA_CONFIG_GUESS_H_INCLUDED_

/////////////////////////////////////////////////////////
//
// This file tries to guess the compiler you are using.
//
// It will error if it can't work it out
// Please contact the omegaonline team if it happens
//
/////////////////////////////////////////////////////////

#ifndef __cplusplus
#error Omega Online insists on C++
#endif

#if defined(_MSC_VER)
// MS Visual Studio
#include "./config-msvc.h"
#elif defined (__clang__)
#include "./config-clang.h"
#elif defined (__BORLANDC__)
// Borland C++ Builder
#include "./config-borland.h"
#elif defined (__GNUC__) || defined(__CDT_PARSER__)
// Keep this last, as a lot of compilers pretend...
#include "./config-gcc.h"
#else
#error Failed to guess your compiler.  Please contact the omegaonline developers.
#endif

#endif // OMEGA_CONFIG_GUESS_H_INCLUDED_
