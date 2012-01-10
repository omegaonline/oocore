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
		
		string_t(const char* sz, size_t length = npos);
		
		~string_t();

		template <size_t S>
		static string_t constant(const char (&arr)[S]);
		static string_t constant(const char* sz, size_t len);
		
		string_t& operator = (const string_t& s);
		
		string_t& operator += (const string_t& s);
		string_t& operator += (const char* sz);
		string_t& operator += (char c);

		const char* c_str() const;
		const char operator[](size_t i) const;

		template <typename T> bool operator == (T v) const { return Compare(v) == 0; }
		template <typename T> bool operator != (T v) const { return Compare(v) != 0; }
		template <typename T> bool operator < (T v) const  { return Compare(v) < 0;  }
		template <typename T> bool operator <= (T v) const { return Compare(v) <= 0; }
		template <typename T> bool operator > (T v) const  { return Compare(v) > 0;  }
		template <typename T> bool operator >= (T v) const { return Compare(v) >= 0; }

		int Compare(const string_t& s, size_t pos = 0, size_t length = npos) const;
		int Compare(const char* sz) const;

		bool IsEmpty() const;
		bool operator !() const;

		size_t Length() const;
		string_t& Clear();

		size_t Find(char c, size_t pos = 0) const;
		size_t Find(const string_t& str, size_t pos = 0) const;
		size_t FindNot(char c, size_t pos = 0) const;
		size_t ReverseFind(char c, size_t pos = npos) const;
		size_t FindOneOf(const string_t& str, size_t pos = 0) const;
		size_t FindNotOf(const string_t& str, size_t pos = 0) const;

		string_t Left(size_t length) const;
		string_t Mid(size_t start, size_t length = npos) const;
		string_t Right(size_t length) const;

		string_t TrimLeft(const string_t& str) const;
		string_t TrimRight(const string_t& str) const;

		template <typename T>
		T ToNumber() const;

		template <typename T>
		string_t& operator %= (T rhs);

		static int64_t strto64(const string_t& str, size_t& end_pos, unsigned int base);
		static uint64_t strtou64(const string_t& str, size_t& end_pos, unsigned int base);
		static float8_t strtod(const string_t& str, size_t& end_pos);

	private:
		struct handle_t
		{
			int unused;
		}* m_handle;

		explicit string_t(handle_t*, bool addref);

		static handle_t* addref(handle_t* h, bool own);
		static void release(handle_t* h);

		friend struct Omega::System::Internal::string_t_safe_type;

#if !defined(NDEBUG)
		const char* m_debug_value;
#endif
	};

	namespace Formatting
	{
		string_t ToString(const string_t& val, const string_t& = string_t());
		string_t ToString(const char* val, const string_t& = string_t());
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

	namespace System
	{
		namespace Internal
		{
			string_t get_text(const char* sz);
		}
	}
}

Omega::string_t operator + (const Omega::string_t& lhs, const Omega::string_t& rhs);
Omega::string_t operator + (const char* lhs, const Omega::string_t& rhs);
Omega::string_t operator + (const Omega::string_t& lhs, const char* rhs);
Omega::string_t operator + (char lhs, const Omega::string_t& rhs);
Omega::string_t operator + (const Omega::string_t& lhs, char rhs);

template <typename T>
Omega::Formatting::formatter_t operator % (const Omega::string_t& lhs, T rhs);
Omega::Formatting::formatter_t operator % (const char* lhs, const Omega::string_t& rhs);

#endif // OMEGA_STRING_H_INCLUDED_
