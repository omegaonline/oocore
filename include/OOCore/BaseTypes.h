///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
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

#ifndef OOCORE_BASE_TYPES_H_INCLUDED_
#define OOCORE_BASE_TYPES_H_INCLUDED_

/////////////////////////////////////////////////////////
//
// This file tries to work out some basic type definitions.
//
// It does not use config-guess.h
//
// It will error if it can't work it out
// Please contact the omegaonline team if it happens
//
/////////////////////////////////////////////////////////

namespace Omega
{
	typedef bool bool_t;
	typedef unsigned char byte_t;
}

#if defined(_MSC_VER)
// MS Visual Studio
namespace Omega
{
	typedef __int16 int16_t;
	typedef unsigned __int16 uint16_t;
	typedef __int32 int32_t;
	typedef unsigned __int32 uint32_t;
	typedef __int64 int64_t;
	typedef unsigned __int64 uint64_t;
	typedef float float4_t;
	typedef double float8_t;
}
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
namespace Omega
{
	using ::int16_t;
	using ::uint16_t;
	using ::int32_t;
	using ::uint32_t;
	using ::int64_t;
	using ::uint64_t;
	typedef float float4_t;
	typedef double float8_t;
}
#else
#error Failed to guess your compiler.  Please contact the omegaonline developers.
#endif

#endif // OOCORE_BASE_TYPES_H_INCLUDED_
