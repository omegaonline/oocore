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
	class string_t
	{
	public:
		inline string_t();
		inline string_t(const string_t& s);
		inline string_t(const char* sz, bool_t bUTF8);
		inline string_t(const wchar_t* wsz, size_t length = npos);
		inline ~string_t();

		inline string_t& operator = (const string_t& s);
		inline string_t& operator = (const wchar_t* wsz);

		inline const wchar_t* c_str() const;
		const wchar_t operator[](size_t i) const
		{ return c_str()[i]; }

		inline size_t ToUTF8(char* sz, size_t size) const;
		inline std::string ToUTF8() const;

		template <class T>
		bool operator == (T t) const
		{ return Compare(t) == 0; }

		template <class T>
		bool operator != (T t) const
		{ return Compare(t) != 0; }

		template <class T>
		bool operator < (T t) const
		{ return Compare(t) < 0; }

		template <class T>
		bool operator > (T t) const
		{ return Compare(t) > 0; }

		inline string_t& operator += (const string_t& s);
		inline string_t& operator += (const wchar_t* wsz);

		inline int Compare(const string_t& s) const;
		inline int Compare(const wchar_t* sz) const;
		inline int CompareNoCase(const string_t& s) const;
		inline int CompareNoCase(const wchar_t* sz) const;
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
		inline string_t TrimLeft(const wchar_t* sz) const;
		inline string_t TrimRight(wchar_t c = L' ') const;
		inline string_t TrimRight(const wchar_t* sz) const;

		inline static string_t Format(const wchar_t* pszFormat, ...);

		static const size_t npos = size_t(-1);

	private:
		struct handle_t
		{
			int unused;
		}* m_handle;

		inline explicit string_t(handle_t*);

#ifdef OMEGA_DEBUG
		const wchar_t* m_debug_value;
#endif
	};

	struct guid_t
	{
		uint32_t	Data1;
		uint16_t	Data2;
		uint16_t	Data3;
		byte_t		Data4[8];

		inline bool operator==(const guid_t& rhs) const;
		inline bool operator==(const string_t& str) const;
		inline bool operator!=(const guid_t& rhs) const;
		inline bool operator<(const guid_t& rhs) const;
		inline bool operator>(const guid_t& rhs) const;
		inline int Compare(const guid_t& rhs) const;

		inline string_t ToString() const;
		inline static guid_t FromString(const wchar_t* sz);
		inline static guid_t FromString(const string_t& str);
		inline static guid_t Create();
		inline static const guid_t& Null()
		{
			static const guid_t sNull = {0,0,0,{0,0,0,0,0,0,0,0}};
			return sNull;
		}

#ifdef OMEGA_HAS_UUIDOF
		inline static const guid_t& FromUuidof(const _GUID& rhs)
		{
			return *reinterpret_cast<const guid_t*>(&rhs);
		}
#endif
	};

	namespace System
	{
		namespace MetaInfo
		{
			template <class T> struct default_value
			{
				static T value()
				{
					static T v = T();
					return v;
				}
			};

			template <class T> struct default_value<T&>
			{
				static T value()
				{
					return default_value<T>::value();
				}
			};

			// MSVC gets twitchy about size_t/uint32_t
			#if defined(_MSC_VER)
			template <> struct default_value<uint32_t>
			{
				static uint32_t value()
				{
					static uint32_t v;
					return v;
				}
			};
			#endif

			template <class T> struct remove_const
			{
				typedef T type;
			};

			template <class T> struct remove_const<const T>
			{
				typedef T type;
			};

			template <class T> struct remove_const<T&>
			{
				typedef typename remove_const<T>::type& type;
			};

			template <class T> struct remove_const<T*>
			{
				typedef typename remove_const<T>::type* type;
			};
		}
	}
}

inline Omega::string_t operator + (const Omega::string_t& lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

inline Omega::string_t operator + (const Omega::string_t& lhs, const wchar_t* rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

inline Omega::string_t operator + (const wchar_t* lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

#endif // OMEGA_TYPES_H_INCLUDED_
