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

#ifndef OOBASE_BYTE_SWAP_H_INCLUDED_
#define OOBASE_BYTE_SWAP_H_INCLUDED_

#include "config-base.h"

#if defined(_MSC_VER) && defined(_W64)
// Turn off stupid 64-bit warnings..
#pragma warning(push)
#pragma warning(disable: 4311)
#pragma warning(disable: 4312)
#endif

#if defined(_MSC_VER)

#define FAST_BYTESWAP_2(x) _byteswap_ushort((unsigned short)(x)) 
#define FAST_BYTESWAP_4(x) _byteswap_ulong((unsigned long)(x))
#define FAST_BYTESWAP_8(x) _byteswap_uint64((unsigned __int64)(x))

#else

#if defined(HAVE___BUILTIN_BSWAP32)
#define FAST_BYTESWAP_4(x) __builtin_bswap32((long)(x))
#endif

#if defined(HAVE___BUILTIN_BSWAP64)
#define FAST_BYTESWAP_8(x) __builtin_bswap64((long long)(x))
#endif

#endif

namespace OOBase
{
	namespace detail
	{
		template <const size_t S>
		struct byte_swapper;
	}

	template <typename T>
	T byte_swap(const T& val)
	{
		return detail::byte_swapper<sizeof(T)>::byte_swap(val);
	}

	namespace detail
	{
		template <>
		struct byte_swapper<1>
		{
			template <typename T>
			static T byte_swap(T val)
			{
				return val;
			}
		};

		template <>
		struct byte_swapper<2>
		{
			template <typename T>
			static T byte_swap(T val)
			{
	#if defined(FAST_BYTESWAP_2)
				return (T)(FAST_BYTESWAP_2(val));
	#else
				char* v = (char*)&val;
				char r[2];
				r[0] = v[1];
				r[1] = v[0];
				return *(T*)r;
	#endif
			}
		};

		template <>
		struct byte_swapper<4>
		{
			template <typename T>
			static T byte_swap(T val)
			{
	#if defined(FAST_BYTESWAP_4)
				return (T)(FAST_BYTESWAP_4(val));
	#else
				char* v = (char*)&val;
				char r[4];
				r[0] = v[3];
				r[1] = v[2];
				r[2] = v[1];
				r[3] = v[0];
				return *(T*)r;
	#endif
			}
		};

		template <>
		struct byte_swapper<8>
		{
			template <typename T>
			static T byte_swap(T val)
			{
	#if defined(FAST_BYTESWAP_8)
				return (T)(FAST_BYTESWAP_8(val));
	#else
				char* v = (char*)&val;
				char r[8];
				r[0] = v[7];
				r[1] = v[6];
				r[2] = v[5];
				r[3] = v[4];
				r[4] = v[3];
				r[5] = v[2];
				r[6] = v[1];
				r[7] = v[0];
				return *(T*)r;
	#endif
			}
		};
	}
}

#if defined(_MSC_VER) && defined(_W64)
#pragma warning(pop)
#endif

#endif // OOBASE_BYTE_SWAP_H_INCLUDED_
