///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
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

#ifndef OMEGA_STRING_H_INCLUDED_
#define OMEGA_STRING_H_INCLUDED_

#include <string>

namespace Omega
{
	// Forward declare friend types
	namespace System
	{
		namespace Internal
		{
			struct string_t_safe_type;
		}
	}

	namespace Formatting
	{
		class formatter_t;
	}

	class string_t
	{
	public:
		static const size_t npos = size_t(-1);

		string_t();
		string_t(const string_t& s);

		template <size_t S>
		string_t(const wchar_t (&arr)[S], bool copy = false);
		string_t(const wchar_t (&arr)[1]);
		string_t(const wchar_t* wsz, size_t length, bool copy = true);

		string_t(const char* sz, bool bUTF8, size_t length = npos);

		~string_t();

		string_t& operator = (const string_t& s);

		string_t& operator += (const string_t& s);
		string_t& operator += (const wchar_t* wsz);
		string_t& operator += (wchar_t c);

		const wchar_t* c_wstr() const;
		const wchar_t operator[](size_t i) const
		{
			return c_wstr()[i];
		}

		const char* ToUTF8(size_t* len = NULL) const;
		const char* ToNative(size_t* len = NULL) const;

		template <typename T> bool operator == (T v) const { return Compare(v) == 0; }
		template <typename T> bool operator != (T v) const { return Compare(v) != 0; }
		template <typename T> bool operator < (T v) const  { return Compare(v) < 0;  }
		template <typename T> bool operator <= (T v) const { return Compare(v) <= 0; }
		template <typename T> bool operator > (T v) const  { return Compare(v) > 0;  }
		template <typename T> bool operator >= (T v) const { return Compare(v) >= 0; }

		int Compare(const string_t& s) const;
		int Compare(const string_t& s, size_t pos, size_t length = npos, bool bIgnoreCase = false) const;
		int Compare(const wchar_t* wsz, size_t pos = 0, size_t length = npos, bool bIgnoreCase = false) const;

		bool IsEmpty() const;
		bool operator !() const
		{
			return IsEmpty();
		}

		size_t Length() const;
		string_t& Clear();

		size_t Find(wchar_t c, size_t pos = 0, bool bIgnoreCase = false) const;
		size_t Find(const string_t& str, size_t pos = 0, bool bIgnoreCase = false) const;
		size_t FindNot(wchar_t c, size_t pos = 0, bool bIgnoreCase = false) const;
		size_t ReverseFind(wchar_t c, size_t pos = npos, bool bIgnoreCase = false) const;
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
		T ToNumber() const;

		template <typename T>
		string_t& operator %= (const T& rhs)
		{
			return (*this = (*this % rhs));
		}

		static int64_t wcsto64(const string_t& str, size_t& end_pos, unsigned int base);
		static uint64_t wcstou64(const string_t& str, size_t& end_pos, unsigned int base);
		static float8_t wcstod(const string_t& str, size_t& end_pos);

	private:
		struct handle_t
		{
			int unused;
		}* m_handle;

		explicit string_t(handle_t*, bool addref);

		static handle_t* addref(handle_t* h, bool own);
		static void release(handle_t* h);

		friend struct Omega::System::Internal::string_t_safe_type;

#ifdef OMEGA_DEBUG
		const wchar_t* m_debug_value;
#endif
	};

	namespace Formatting
	{
		string_t ToString(const string_t& val, const string_t& = string_t());
		string_t ToString(const wchar_t* val, const string_t& = string_t());
		string_t ToString(bool_t val, const string_t& strFormat = string_t());

		template <typename T>
		string_t ToString(T val, const string_t& strFormat = string_t());

		class formatter_t
		{
		public:
			formatter_t(const string_t& format);

			formatter_t(const formatter_t& rhs) : m_handle(clone_handle(rhs))
			{ }

			formatter_t& operator = (const formatter_t& rhs)
			{
				if (this != &rhs)
				{
					free_handle(m_handle);
					m_handle = clone_handle(rhs);
				}
				return *this;
			}

			~formatter_t();

			template <typename T>
			formatter_t& operator % (const T& rhs);

			template <typename T>
			formatter_t operator % (const T& rhs) const
			{
				return formatter_t(*this) % rhs;
			}

			operator string_t() const;

			template <typename T>
			bool operator == (const T& v) const
			{
				return (static_cast<const string_t&>(*this) == v);
			}

		private:
			struct handle_t
			{
				int unused;
			}* m_handle;

			static handle_t* clone_handle(const formatter_t& rhs);
			static void free_handle(handle_t* h);
		};
	}
}

Omega::string_t operator + (const Omega::string_t& lhs, const Omega::string_t& rhs);
Omega::string_t operator + (const wchar_t* lhs, const Omega::string_t& rhs);
Omega::string_t operator + (const Omega::string_t& lhs, const wchar_t* rhs);
Omega::string_t operator + (wchar_t lhs, const Omega::string_t& rhs);
Omega::string_t operator + (const Omega::string_t& lhs, wchar_t rhs);

template <typename T>
Omega::Formatting::formatter_t operator % (const Omega::string_t& lhs, const T& rhs);
Omega::Formatting::formatter_t operator % (const wchar_t* lhs, const Omega::string_t& rhs);

#endif // OMEGA_STRING_H_INCLUDED_
