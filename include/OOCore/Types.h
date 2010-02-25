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
		static const size_t npos = size_t(-1);

		string_t();
		string_t(const string_t& s);
		string_t(const wchar_t* wsz, size_t length = npos);
		string_t(const char* sz, bool bUTF8, size_t length = npos);
		
		~string_t();

		string_t& operator = (const string_t& s);
		string_t& operator = (const wchar_t* wsz);

		string_t& operator += (const string_t& s);
		string_t& operator += (wchar_t c);

		const wchar_t* c_str() const;
		const wchar_t operator[](size_t i) const { return c_str()[i]; }

		size_t ToUTF8(char* sz, size_t size) const;
		std::string ToUTF8() const;

		template <typename T> bool operator == (T v) const { return Compare(v) == 0; }
		template <typename T> bool operator != (T v) const { return Compare(v) != 0; }
		template <typename T> bool operator < (T v) const { return Compare(v) < 0; }
		template <typename T> bool operator <= (T v) const { return Compare(v) <= 0; }
		template <typename T> bool operator > (T v) const { return Compare(v) > 0; }
		template <typename T> bool operator >= (T v) const { return Compare(v) >= 0; }

		int Compare(const string_t& s, size_t pos = 0, size_t length = npos, bool bIgnoreCase = false) const;
		int Compare(const wchar_t* wsz, size_t pos = 0, size_t length = npos, bool bIgnoreCase = false) const;
		
		bool IsEmpty() const;
		size_t Length() const;
		string_t& Clear();

		size_t Find(wchar_t c, size_t pos = 0, bool bIgnoreCase = false) const;
		size_t FindNot(wchar_t c, size_t pos = 0, bool bIgnoreCase = false) const;
		size_t ReverseFind(wchar_t c, size_t pos = npos, bool bIgnoreCase = false) const;
		size_t Find(const string_t& str, size_t pos = 0, bool bIgnoreCase = false) const;
		size_t FindOneOf(const string_t& str, size_t pos = 0, bool bIgnoreCase = false) const;
		size_t FindNotOf(const string_t& str, size_t pos = 0, bool bIgnoreCase = false) const;		
		
		string_t Left(size_t length) const;
		string_t Mid(size_t start, size_t length = npos) const;
		string_t Right(size_t length) const;
		
		string_t ToLower() const;
		string_t ToUpper() const;

		string_t TrimLeft(wchar_t c = L' ') const;
		string_t TrimLeft(const string_t& str) const;
		string_t TrimRight(wchar_t c = L' ') const;
		string_t TrimRight(const string_t& str) const;

		template <typename T>
		string_t& operator %= (T val);
		
	private:
		struct handle_t
		{
			int unused;
		}* m_handle;

		explicit string_t(handle_t*);

		static void addref(handle_t* h);
		static void release(handle_t* h);

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

		bool operator == (const guid_t& rhs) const { return Compare(rhs) == 0; }
		bool operator != (const guid_t& rhs) const { return Compare(rhs) != 0; }
		bool operator < (const guid_t& rhs) const { return Compare(rhs) < 0; }
		bool operator <= (const guid_t& rhs) const { return Compare(rhs) <= 0; }
		bool operator > (const guid_t& rhs) const { return Compare(rhs) > 0; }
		bool operator >= (const guid_t& rhs) const { return Compare(rhs) >= 0; }
		int Compare(const guid_t& rhs) const;

		static guid_t Create();

		static bool FromString(const string_t& str, guid_t& guid);
		static guid_t FromString(const string_t& str);

		static const guid_t& Null()
		{
			static const guid_base_t sbNull = {0,0,0,{0,0,0,0,0,0,0,0}};
			static const guid_t sNull(sbNull);
			return sNull;
		}

		string_t ToString(const string_t& strFormat = L"") const;
	};

	namespace Formatting
	{
		string_t ToString(const string_t& val, const string_t&);
		string_t ToString(const wchar_t* val, const string_t&);
		string_t ToString(bool_t val, const string_t& strFormat);

		template <typename T>
		string_t ToString(T val, const string_t& strFormat);
	}

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

			template <> struct optimal_param<const bool>
			{
				typedef const bool type;
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

Omega::string_t operator + (const Omega::string_t& lhs, const Omega::string_t& rhs);
Omega::string_t operator + (const wchar_t* lhs, const Omega::string_t& rhs);
Omega::string_t operator + (const Omega::string_t& lhs, const wchar_t* rhs);
Omega::string_t operator + (wchar_t lhs, const Omega::string_t& rhs);
Omega::string_t operator + (const Omega::string_t& lhs, wchar_t rhs);

template <typename T>
Omega::string_t operator % (const Omega::string_t& lhs, const T& rhs);

#endif // OMEGA_TYPES_H_INCLUDED_
