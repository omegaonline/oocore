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

#ifndef OMEGA_TYPES_H_INCLUDED_
#define OMEGA_TYPES_H_INCLUDED_

#include "./base_types.h"
#include "./string.h"

namespace Omega
{

#if defined(_WIN32)
	typedef struct _GUID guid_base_t;
#else
	struct guid_base_t
	{
		uint32_t    Data1;
		uint16_t    Data2;
		uint16_t    Data3;
		byte_t      Data4[8];
	};
#endif

	struct guid_t : public guid_base_t
	{
		guid_t(const guid_base_t& rhs = guid_t::Null()) : guid_base_t(rhs)
		{}

		guid_t(const char* wsz);
		guid_t(const string_t& str);

		bool operator == (const guid_t& rhs) const { return Compare(rhs) == 0; }
		bool operator != (const guid_t& rhs) const { return Compare(rhs) != 0; }
		bool operator < (const guid_t& rhs) const { return Compare(rhs) < 0; }
		bool operator <= (const guid_t& rhs) const { return Compare(rhs) <= 0; }
		bool operator > (const guid_t& rhs) const { return Compare(rhs) > 0; }
		bool operator >= (const guid_t& rhs) const { return Compare(rhs) >= 0; }
		int Compare(const guid_t& rhs) const;

		static guid_t Create();

		static bool FromString(const char* wsz, guid_t& guid);
		static bool FromString(const string_t& str, guid_t& guid);

		static const guid_t& Null()
		{
			static const guid_base_t sbNull = {0,0,0,{0,0,0,0,0,0,0,0}};
			static const guid_t sNull(sbNull);
			return sNull;
		}

		string_t ToString(const string_t& strFormat = string_t()) const;
	};

	namespace System
	{
		namespace Internal
		{
			template <typename T> struct default_value
			{
				static T value()
				{
					return T();
				}
			};

			template <typename T> struct default_value<T*>
			{
				static T* value()
				{
					return NULL;
				}
			};

			template <> struct default_value<guid_t>
			{
				static const guid_t& value()
				{
					return guid_t::Null();
				}
			};

			template <typename T> struct remove_const
			{
				typedef T type;
			};

			template <typename T> struct remove_const<const T>
			{
				typedef T type;
			};

			template <typename T> struct remove_const<T*>
			{
				typedef typename remove_const<T>::type* type;
			};

			template <typename T> struct remove_const<T&>
			{
				typedef typename remove_const<T>::type& type;
			};

			template <typename T> struct remove_ref
			{
				typedef T type;
			};

			template <typename T> struct remove_ref<T&>
			{
				typedef T type;
			};

			template <bool, typename True, typename False>
			struct if_else_t
			{
				typedef False result;
			};

			template <typename True, typename False>
			struct if_else_t<true,True,False>
			{
				typedef True result;
			};

			// These are the C ABI compliant types
			template <typename T> struct is_c_abi
			{
				enum { result = 0 };
			};

			// These are defined by the C ABI
			template <> struct is_c_abi<char>
			{
				enum { result = 1 };
			};
			template <> struct is_c_abi<signed char>
			{
				enum { result = 1 };
			};
			template <> struct is_c_abi<unsigned char>
			{
				enum { result = 1 };
			};
			template <> struct is_c_abi<short>
			{
				enum { result = 1 };
			};
			template <> struct is_c_abi<unsigned short>
			{
				enum { result = 1 };
			};
			template <> struct is_c_abi<int>
			{
				enum { result = 1 };
			};
			template <> struct is_c_abi<unsigned int>
			{
				enum { result = 1 };
			};
			template <> struct is_c_abi<long>
			{
				enum { result = 1 };
			};
			template <> struct is_c_abi<unsigned long>
			{
				enum { result = 1 };
			};
			template <> struct is_c_abi<long long>
			{
				enum { result = 1 };
			};
			template <> struct is_c_abi<unsigned long long>
			{
				enum { result = 1 };
			};
			template <> struct is_c_abi<float>
			{
				enum { result = 1 };
			};
			template <> struct is_c_abi<double>
			{
				enum { result = 1 };
			};
			
			// Simple structures
			template <> struct is_c_abi<guid_base_t>
			{
				enum { result = 1 };
			};

			// Pointers are also C ABI compliant
			template <> struct is_c_abi<void*>
			{
				enum { result = 1 };
			};
			template <typename T> struct is_c_abi<T*>
			{
				enum { result = is_c_abi<T>::result };
			};

			// Const can be safely ignored
			template <typename T> struct is_c_abi<const T>
			{
				enum { result = is_c_abi<T>::result };
			};

			// Volatile can be safely ignored
			template <typename T> struct is_c_abi<volatile T>
			{
				enum { result = is_c_abi<T>::result };
			};

			// Optimal parameter passing
			template <typename T> struct optimal_param
			{
				// If C-ABI compliant, then pass by value, else by const ref
				typedef typename if_else_t<is_c_abi<T>::result,T,const typename remove_const<T>::type&>::result type;
			};

			template <> struct optimal_param<bool>
			{
				typedef bool type;
			};

			template <> struct optimal_param<guid_base_t>
			{
				typedef const guid_base_t& type;
			};

			template <typename T> struct optimal_param<T&>
			{
				typedef T& type;
			};

			template <typename T> struct optimal_param<T*>
			{
				typedef T* type;
			};

			template <typename T> struct optimal_param<const T>
			{
				typedef typename optimal_param<T>::type type;
			};
		}
	}
}

#endif // OMEGA_TYPES_H_INCLUDED_
