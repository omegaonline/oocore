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

#include "BaseTypes.h"

namespace Omega
{
	namespace System
	{
		namespace MetaInfo
		{
			struct string_t_safe_type;
		}
	}

	class string_t
	{
	public:
		inline string_t();
		inline string_t(const string_t& s);
		inline string_t(const char* sz, bool bUTF8);
		inline string_t(const wchar_t* wsz, size_t length = npos);
		inline ~string_t();

		inline string_t& operator = (const string_t& s);
		inline string_t& operator = (const wchar_t* sz);

		inline const wchar_t* c_str() const;
		const wchar_t operator[](size_t i) const
		{ return c_str()[i]; }

		inline size_t ToUTF8(char* sz, size_t size) const;
		inline std::string ToUTF8() const;

		bool operator == (const string_t& s) const { return Compare(s) == 0; }
		bool operator != (const string_t& s) const { return Compare(s) != 0; }
		bool operator < (const string_t& s) const { return Compare(s) < 0; }
		bool operator <= (const string_t& s) const { return Compare(s) <= 0; }
		bool operator > (const string_t& s) const { return Compare(s) > 0; }
		bool operator >= (const string_t& s) const { return Compare(s) >= 0; }

		inline string_t& operator += (const string_t& s);

		inline int Compare(const string_t& s) const;
		inline int CompareNoCase(const string_t& s) const;
		inline bool IsEmpty() const;
		inline size_t Length() const;
		inline size_t Find(const string_t& str, size_t pos = 0, bool bIgnoreCase = false) const;
		inline size_t Find(wchar_t c, size_t pos = 0, bool bIgnoreCase = false) const;
		inline size_t ReverseFind(wchar_t c, size_t pos = npos, bool bIgnoreCase = false) const;
		inline string_t Left(size_t length) const;
		inline string_t Mid(size_t start, size_t length = npos) const;
		inline string_t Right(size_t length) const;
		inline string_t& Clear();
		inline string_t ToLower() const;
		inline string_t ToUpper() const;
		inline string_t TrimLeft(wchar_t c = L' ') const;
		inline string_t TrimLeft(const string_t& str) const;
		inline string_t TrimRight(wchar_t c = L' ') const;
		inline string_t TrimRight(const string_t& str) const;

		inline static string_t Format(const wchar_t* pszFormat, ...);

		static const size_t npos = size_t(-1);

	private:
		struct handle_t
		{
			int unused;
		}* m_handle;

		inline explicit string_t(handle_t*);

		inline static void addref(handle_t* h);
		inline static void release(handle_t* h);

		friend struct Omega::System::MetaInfo::string_t_safe_type;

#ifdef OMEGA_DEBUG
		const wchar_t* m_debug_value;
#endif
	};

#if defined(_WIN32)
	typedef struct _GUID guid_base_t;
#else
	struct guid_base_t
	{
		uint32_t	Data1;
		uint16_t	Data2;
		uint16_t	Data3;
		byte_t		Data4[8];
	};
#endif

	struct guid_t : public guid_base_t
	{
		guid_t()
		{}

		guid_t(const guid_base_t& rhs) : guid_base_t(rhs)
		{}

		inline bool operator==(const guid_t& rhs) const;
		inline bool operator==(const string_t& str) const;
		inline bool operator!=(const guid_t& rhs) const;
		inline bool operator<(const guid_t& rhs) const;
		inline bool operator>(const guid_t& rhs) const;
		inline int Compare(const guid_t& rhs) const;

		inline string_t ToString() const;
		inline static bool FromString(const wchar_t* sz, guid_t& guid);
		inline static bool FromString(const string_t& str, guid_t& guid);
		inline static guid_t FromString(const wchar_t* sz);
		inline static guid_t FromString(const string_t& str);
		inline static guid_t Create();
		inline static const guid_t& Null()
		{
			static const guid_base_t sbNull = {0,0,0,{0,0,0,0,0,0,0,0}};
			static const guid_t sNull(sbNull);
			return sNull;
		}
	};

	namespace System
	{
		namespace MetaInfo
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
					return static_cast<T*>(0);
				}
			};

			template <> struct default_value<guid_t>
			{
				static const guid_t& value()
				{
					return guid_t::Null();
				}
			};

			// MSVC gets twitchy about size_t/uint32_t
			#if defined(_MSC_VER)
			template <> struct default_value<uint32_t>
			{
				static uint32_t value()
				{
					return 0;
				}
			};
			#endif

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
			template <> struct is_c_abi<void> { enum { result = 1 }; };
			template <> struct is_c_abi<char> { enum { result = 1 }; };
			template <> struct is_c_abi<signed char> { enum { result = 1 }; };
			template <> struct is_c_abi<unsigned char> { enum { result = 1 }; };
			template <> struct is_c_abi<signed short> { enum { result = 1 }; };
			template <> struct is_c_abi<unsigned short> { enum { result = 1 }; };
			template <> struct is_c_abi<signed int> { enum { result = 1 }; };
			template <> struct is_c_abi<unsigned int> { enum { result = 1 }; };
			template <> struct is_c_abi<signed long> { enum { result = 1 }; };
			template <> struct is_c_abi<unsigned long> { enum { result = 1 }; };
			template <> struct is_c_abi<float> { enum { result = 1 }; };
			template <> struct is_c_abi<double> { enum { result = 1 }; };

			// Platform specific - therefore safe
			template <> struct is_c_abi<wchar_t> { enum { result = 1 }; };

			// Fairly sure about these
			template <> struct is_c_abi<int64_t> { enum { result = 1 }; };
			template <> struct is_c_abi<uint64_t> { enum { result = 1 }; };

			// Simple structures
			template <> struct is_c_abi<guid_base_t> { enum { result = 1 }; };

			// Pointers are also C ABI compliant
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

			// Optimal paramater passing
			template <typename T> struct optimal_param
			{
				typedef typename if_else_t<is_c_abi<T>::result,
					typename if_else_t<sizeof(T) <= sizeof(size_t),T,const typename remove_const<T>::type&>::result,
					const typename remove_const<T>::type&
				>::result type;
			};

			template <> struct optimal_param<bool>
			{
				typedef bool type;
			};

			template <typename T> struct optimal_param<T&>
			{
				typedef T& type;
			};

			template <typename T> struct optimal_param<T*>
			{
				typedef T* type;
			};
		}
	}
}

#endif // OMEGA_TYPES_H_INCLUDED_
