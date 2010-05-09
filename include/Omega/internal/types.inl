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

#ifndef OOCORE_TYPES_INL_INCLUDED_
#define OOCORE_TYPES_INL_INCLUDED_

////////////////////////////////////////////////////
// string_t starts here

#ifdef OMEGA_DEBUG
#define OMEGA_DEBUG_STASH_STRING()  m_debug_value = (m_handle ? OOCore_string_t_cast(m_handle) : L"")
#else
#define OMEGA_DEBUG_STASH_STRING()  (void)0
#endif

OOCORE_RAW_EXPORTED_FUNCTION(const wchar_t*,OOCore_string_t_cast,1,((in),const void*,h));

inline Omega::string_t::string_t(handle_t* h, bool bAddref) :
		m_handle(bAddref ? addref(h,false) : h)
{
	OMEGA_DEBUG_STASH_STRING();
}

inline Omega::string_t::string_t() : m_handle(0)
{
	OMEGA_DEBUG_STASH_STRING();
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor1,3,((in),const char*,sz,(in),size_t,len,(in),int,bUTF8));
inline Omega::string_t::string_t(const char* sz, bool bUTF8, size_t length) :
		m_handle(0)
{
	if (sz)
		m_handle = static_cast<handle_t*>(OOCore_string_t__ctor1(sz,length,bUTF8 ? 1 : 0));

	OMEGA_DEBUG_STASH_STRING();
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_addref,2,((in),void*,s1,(in),int,own));
inline Omega::string_t::handle_t* Omega::string_t::addref(handle_t* h, bool own)
{
	if (!h)
		return 0;

	return static_cast<handle_t*>(OOCore_string_t_addref(h,own ? 1 : 0));
}

inline Omega::string_t::string_t(const Omega::string_t& s) :
		m_handle(addref(s.m_handle,false))
{
	OMEGA_DEBUG_STASH_STRING();
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor2,3,((in),const wchar_t*,wsz,(in),size_t,length,(in),int,copy));

template <size_t S>
inline Omega::string_t::string_t(const wchar_t (&arr)[S], bool copy) :
		m_handle(0)
{
	m_handle = static_cast<handle_t*>(OOCore_string_t__ctor2(arr,S-1,copy ? 1 : 0));

	OMEGA_DEBUG_STASH_STRING();
}

inline Omega::string_t::string_t(const wchar_t (&)[1]) :
		m_handle(0)
{
	OMEGA_DEBUG_STASH_STRING();
}

inline Omega::string_t::string_t(const wchar_t* wsz, size_t length, bool copy) :
		m_handle(0)
{
	if (wsz)
		m_handle = static_cast<handle_t*>(OOCore_string_t__ctor2(wsz,length,copy ? 1 : 0));

	OMEGA_DEBUG_STASH_STRING();
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_release,1,((in),void*,h));
inline void Omega::string_t::release(handle_t* h)
{
	if (h)
		OOCore_string_t_release(h);
}

inline Omega::string_t::~string_t()
{
	release(m_handle);
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_assign1,2,((in),void*,h1,(in),const void*,h2));
inline Omega::string_t& Omega::string_t::operator = (const string_t& s)
{
	if (this != &s && m_handle != s.m_handle)
	{
		m_handle = static_cast<handle_t*>(OOCore_string_t_assign1(m_handle,s.m_handle));
		OMEGA_DEBUG_STASH_STRING();
	}
	return *this;
}

inline const wchar_t* Omega::string_t::c_str() const
{
	if (!m_handle)
		return L"";

	return OOCore_string_t_cast(m_handle);
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_toutf8,3,((in),const void*,h,(in),char*,sz,(in),size_t,size));
inline size_t Omega::string_t::ToUTF8(char* sz, size_t size) const
{
	return OOCore_string_t_toutf8(m_handle,sz,size);
}

inline std::string Omega::string_t::ToUTF8() const
{
	std::string str;
	char szBuf[128];
	size_t len = ToUTF8(szBuf,128);
	if (len > 128)
	{
		char* pszBuf = 0;
		OMEGA_NEW(pszBuf,char[len]);
		try
		{
			ToUTF8(pszBuf,len);
		}
		catch (...)
		{
			delete [] pszBuf;
			throw;
		}
		str.assign(pszBuf,len-1);
		delete [] pszBuf;
	}
	else
		str.assign(szBuf,len-1);

	return str;
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_add1,2,((in),void*,h,(in),const void*,h2));
inline Omega::string_t& Omega::string_t::operator += (const string_t& s)
{
	if (s.m_handle)
	{
		m_handle = static_cast<handle_t*>(OOCore_string_t_add1(m_handle,s.m_handle));
		OMEGA_DEBUG_STASH_STRING();
	}
	return *this;
}

inline Omega::string_t& Omega::string_t::operator += (const wchar_t* wsz)
{
	return this->operator +=(string_t(wsz,string_t::npos,false));
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_add2,2,((in),void*,h,(in),wchar_t,c));
inline Omega::string_t& Omega::string_t::operator += (wchar_t c)
{
	m_handle = static_cast<handle_t*>(OOCore_string_t_add2(m_handle,c));
	OMEGA_DEBUG_STASH_STRING();
	return *this;
}

inline int Omega::string_t::Compare(const string_t& s) const
{
	if (&s == this)
		return 0;

	return Compare(s,0,string_t::npos,false);
}

OOCORE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cmp1,5,((in),const void*,h1,(in),const void*,h2,(in),size_t,pos,(in),size_t,length,(in),int,bIgnoreCase));
inline int Omega::string_t::Compare(const string_t& s, size_t pos, size_t length, bool bIgnoreCase) const
{
	if (m_handle == s.m_handle)
	{
		if ((pos == 0 && length == string_t::npos) || !m_handle)
			return 0;
	}

	if (!m_handle)
		return -1;

	if (!s.m_handle)
		return 1;

	return OOCore_string_t_cmp1(m_handle,s.m_handle,pos,length,(bIgnoreCase ? 1 : 0));
}

OOCORE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cmp2,5,((in),const void*,h,(in),const wchar_t*,wsz,(in),size_t,pos,(in),size_t,length,(in),int,bIgnoreCase));
inline int Omega::string_t::Compare(const wchar_t* wsz, size_t pos, size_t length, bool bIgnoreCase) const
{
	if (!m_handle)
	{
		if (!wsz || wsz[0] == L'\0')
			return 0;

		return -1;
	}

	if (!wsz)
		return 1;

	return OOCore_string_t_cmp2(m_handle,wsz,pos,length,(bIgnoreCase ? 1 : 0));
}

inline bool Omega::string_t::IsEmpty() const
{
	return (Length() == 0);
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_len,1,((in),const void*,h));
inline size_t Omega::string_t::Length() const
{
	if (!m_handle)
		return 0;

	return OOCore_string_t_len(m_handle);
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find1,4,((in),const void*,a,(in),wchar_t,b,(in),size_t,c,(in),int,d));
inline size_t Omega::string_t::Find(wchar_t c, size_t pos, bool bIgnoreCase) const
{
	if (!m_handle)
		return string_t::npos;

	return OOCore_string_t_find1(m_handle,c,pos,(bIgnoreCase ? 1 : 0));
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_not,4,((in),const void*,a,(in),wchar_t,b,(in),size_t,c,(in),int,d));
inline size_t Omega::string_t::FindNot(wchar_t c, size_t pos, bool bIgnoreCase) const
{
	if (!m_handle)
		return string_t::npos;

	return OOCore_string_t_find_not(m_handle,c,pos,(bIgnoreCase ? 1 : 0));
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find2,4,((in),const void*,h1,(in),const void*,h2,(in),size_t,s,(in),int,bIgnoreCase));
inline size_t Omega::string_t::Find(const string_t& str, size_t pos, bool bIgnoreCase) const
{
	if (!m_handle || !str.m_handle)
		return string_t::npos;

	return OOCore_string_t_find2(m_handle,str.m_handle,pos,(bIgnoreCase ? 1 : 0));
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_oneof,4,((in),const void*,h1,(in),const void*,h2,(in),size_t,s,(in),int,bIgnoreCase));
inline size_t Omega::string_t::FindOneOf(const string_t& str, size_t pos, bool bIgnoreCase) const
{
	if (!m_handle || !str.m_handle)
		return string_t::npos;

	return OOCore_string_t_find_oneof(m_handle,str.m_handle,pos,(bIgnoreCase ? 1 : 0));
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_notof,4,((in),const void*,h1,(in),const void*,h2,(in),size_t,s,(in),int,bIgnoreCase));
inline size_t Omega::string_t::FindNotOf(const string_t& str, size_t pos, bool bIgnoreCase) const
{
	if (!m_handle || !str.m_handle)
		return string_t::npos;

	return OOCore_string_t_find_notof(m_handle,str.m_handle,pos,(bIgnoreCase ? 1 : 0));
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_rfind,4,((in),const void*,a,(in),wchar_t,b,(in),size_t,c,(in),int,d));
inline size_t Omega::string_t::ReverseFind(wchar_t c, size_t pos, bool bIgnoreCase) const
{
	if (!m_handle)
		return string_t::npos;

	return OOCore_string_t_rfind(m_handle,c,pos,(bIgnoreCase ? 1 : 0));
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_left,2,((in),const void*,a,(in),size_t,b));
inline Omega::string_t Omega::string_t::Left(size_t length) const
{
	if (length == 0 || !m_handle)
		return string_t((handle_t*)0,false);

	return string_t(static_cast<handle_t*>(OOCore_string_t_left(m_handle,length)),false);
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_mid,3,((in),const void*,h,(in),size_t,a,(in),size_t,b));
inline Omega::string_t Omega::string_t::Mid(size_t start, size_t length) const
{
	if (length == 0 || !m_handle)
		return string_t((handle_t*)0,false);

	return string_t(static_cast<handle_t*>(OOCore_string_t_mid(m_handle,start,length)),false);
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_right,2,((in),const void*,a,(in),size_t,b));
inline Omega::string_t Omega::string_t::Right(size_t length) const
{
	if (length == 0 || !m_handle)
		return string_t((handle_t*)0,false);

	return string_t(static_cast<handle_t*>(OOCore_string_t_right(m_handle,length)),false);
}

inline Omega::string_t& Omega::string_t::Clear()
{
	release(m_handle);
	m_handle = 0;

	OMEGA_DEBUG_STASH_STRING();

	return *this;
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_tolower,1,((in),const void*,h));
inline Omega::string_t Omega::string_t::ToLower() const
{
	if (!m_handle)
		return string_t((handle_t*)0,false);

	return string_t(static_cast<handle_t*>(OOCore_string_t_tolower(m_handle)),false);
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_toupper,1,((in),const void*,h));
inline Omega::string_t Omega::string_t::ToUpper() const
{
	if (!m_handle)
		return string_t((handle_t*)0,false);

	return string_t(static_cast<handle_t*>(OOCore_string_t_toupper(m_handle)),false);
}

inline Omega::string_t Omega::string_t::TrimLeft(wchar_t c) const
{
	return Mid(FindNot(c));
}

inline Omega::string_t Omega::string_t::TrimLeft(const string_t& str) const
{
	return Mid(FindNotOf(str));
}

inline Omega::string_t Omega::string_t::TrimRight(wchar_t c) const
{
	const wchar_t* s = c_str();
	const wchar_t* p = s + Length()-1;
	while (p>=s && *p == c)
		--p;

	return Left(p+1-s);
}

inline Omega::string_t Omega::string_t::TrimRight(const string_t& str) const
{
	const wchar_t* s = c_str();
	const wchar_t* p = s + Length()-1;
	while (p>=s && str.Find(*p) != string_t::npos)
		--p;

	return Left(p+1-s);
}

inline Omega::string_t operator + (const Omega::string_t& lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

inline Omega::string_t operator + (const wchar_t* lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t(lhs,Omega::string_t::npos,false) += rhs);
}

inline Omega::string_t operator + (const Omega::string_t& lhs, const wchar_t* rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

inline Omega::string_t operator + (wchar_t lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t(&lhs,1,true) += rhs);
}

inline Omega::string_t operator + (const Omega::string_t& lhs, wchar_t rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

template <typename T>
inline Omega::string_t operator % (const Omega::string_t& lhs, const T& rhs)
{
	return (Omega::string_t(lhs) %= rhs);
}

inline Omega::string_t operator % (const wchar_t* lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t(lhs,Omega::string_t::npos,false) %= rhs);
}

OOCORE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_get_arg,3,((in),size_t,idx,(in_out),void**,s1,(out),void**,fmt));
OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_set_arg,2,((in),void*,s1,(in),void*,arg));

OOCORE_EXPORTED_FUNCTION(Omega::string_t,OOCore_to_string_int_t,3,((in),Omega::int64_t,val,(in),const Omega::string_t&,strFormat,(in),size_t,byte_width));
OOCORE_EXPORTED_FUNCTION(Omega::string_t,OOCore_to_string_uint_t,3,((in),Omega::uint64_t,val,(in),const Omega::string_t&,strFormat,(in),size_t,byte_width));
OOCORE_EXPORTED_FUNCTION(Omega::string_t,OOCore_to_string_float_t,3,((in),Omega::float8_t,val,(in),const Omega::string_t&,strFormat,(in),size_t,byte_width));
OOCORE_EXPORTED_FUNCTION(Omega::string_t,OOCore_to_string_bool_t,2,((in),Omega::bool_t,val,(in),const Omega::string_t&,strFormat));

namespace Omega
{
	namespace System
	{
		namespace Internal
		{
			template <typename T>
			struct integer_formatter_t
			{
				static string_t ToString(T val, const string_t& strFormat = string_t())
				{
					return OOCore_to_string_int_t(val,strFormat,sizeof(T));
				}
			};

			template <typename T>
			struct unsigned_integer_formatter_t
			{
				static string_t ToString(T val, const string_t& strFormat = string_t())
				{
					return OOCore_to_string_uint_t(val,strFormat,sizeof(T));
				}
			};

			template <typename T>
			struct float_formatter_t
			{
				static string_t ToString(T val, const string_t& strFormat = string_t())
				{
					return OOCore_to_string_float_t(val,strFormat,sizeof(T));
				}
			};

			template <>
			struct float_formatter_t<float4_t>
			{
				static string_t ToString(float4_t val, const string_t& strFormat = string_t())
				{
					// NAN does not compare successfully to any other number including itself!
					static const float4_t nan = std::numeric_limits<float4_t>::signaling_NaN();
					if (*reinterpret_cast<const uint32_t*>(&val) == *reinterpret_cast<const uint32_t*>(&nan))
						return OOCore_to_string_float_t(std::numeric_limits<float8_t>::signaling_NaN(),strFormat,sizeof(float4_t));
					else
						return OOCore_to_string_float_t(val,strFormat,sizeof(float4_t));
				}
			};

			template <typename T>
			struct to_string_member_formatter_t
			{
				static string_t ToString(const T& val, const string_t& strFormat = string_t())
				{
					return val.ToString(strFormat);
				}
			};

			template <typename T>
			struct formatter_t
			{
				typedef typename if_else_t<std::numeric_limits<T>::is_specialized,
					typename if_else_t<std::numeric_limits<T>::is_integer,
						typename if_else_t<std::numeric_limits<T>::is_signed,integer_formatter_t<T>,unsigned_integer_formatter_t<T> >::result,
						float_formatter_t<T>
					>::result,
					to_string_member_formatter_t<T>
				>::result type;
			};

			template <typename T>
			struct formatter_t<const T>
			{
				typedef typename formatter_t<T>::type type;
			};

			template <typename T>
			struct formatter_t<T&>
			{
				typedef typename formatter_t<T>::type type;
			};

			template <typename T, size_t S>
			struct formatter_t<T[S]>
			{
				typedef typename formatter_t<T*>::type type;
			};

			template <typename T>
			struct formatter_t<const T*>
			{
				typedef typename formatter_t<T*>::type type;
			};

			// Don't pass pointers
			template <typename T> struct formatter_t<T*>;

			// Long doubles are not compiler agnostic...
			template <> struct formatter_t<long double>;
		}
	}
}

inline Omega::string_t Omega::Formatting::ToString(const Omega::string_t& val, const Omega::string_t& strFormat)
{
	if (!strFormat.IsEmpty())
		throw Formatting::IFormattingException::Create(L"Invalid string_t format string: {0}" % strFormat);

	return val;
}

inline Omega::string_t Omega::Formatting::ToString(const wchar_t* val, const Omega::string_t& strFormat)
{
	return ToString(string_t(val,string_t::npos),strFormat);
}

inline Omega::string_t Omega::Formatting::ToString(Omega::bool_t val, const Omega::string_t& strFormat)
{
	return OOCore_to_string_bool_t(val,strFormat);
}

template <typename T>
inline Omega::string_t Omega::Formatting::ToString(T val, const Omega::string_t& strFormat)
{
	return System::Internal::formatter_t<T>::type::ToString(val,strFormat);
}

template <typename T>
inline Omega::string_t& Omega::string_t::operator %= (T val)
{
	for (size_t index = 0;; ++index)
	{
		void* format = 0;
		void* h = m_handle;

		if (!OOCore_string_t_get_arg(index,&h,&format))
			break;

		m_handle = static_cast<handle_t*>(h);

		string_t strVal = Formatting::ToString(val,string_t(static_cast<handle_t*>(format),false));
		OOCore_string_t_set_arg(m_handle,strVal.m_handle);
	}

	OMEGA_DEBUG_STASH_STRING();
	return *this;
}

////////////////////////////////////////////////////
// guid_t starts here

inline int Omega::guid_t::Compare(const guid_t& rhs) const
{
	if (Data1 != rhs.Data1)
		return (Data1 < rhs.Data1 ? -1 : 1);
	else if (Data2 != rhs.Data2)
		return (Data2 < rhs.Data2 ? -1 : 1);
	else if (Data3 != rhs.Data3)
		return (Data3 < rhs.Data3 ? -1 : 1);
	else if (*reinterpret_cast<const uint64_t*>(Data4) != *reinterpret_cast<const uint64_t*>(rhs.Data4))
	{
		for (int i=0; i<8; ++i)
		{
			if (Data4[i] != rhs.Data4[i])
				return (Data4[i] < rhs.Data4[i] ? -1 : 1);
		}
	}

	return 0;
}

OOCORE_EXPORTED_FUNCTION(Omega::string_t,OOCore_guid_t_to_string,2,((in),const Omega::guid_t&,guid,(in),const Omega::string_t&,strFormat));
inline Omega::string_t Omega::guid_t::ToString(const Omega::string_t& strFormat) const
{
	return OOCore_guid_t_to_string(*this,strFormat);
}

OOCORE_EXPORTED_FUNCTION(int,OOCore_guid_t_from_string,2,((in),const Omega::string_t&,str,(out),Omega::guid_t&,guid));
inline bool Omega::guid_t::FromString(const string_t& str, Omega::guid_t& guid)
{
	return (OOCore_guid_t_from_string(str,guid) != 0);
}

inline Omega::guid_t::guid_t(const string_t& str)
{
	if (!guid_t::FromString(str,*this))
		throw Formatting::IFormattingException::Create(L"{0} is not an Omega::guid_t string representation" % str);
}

OOCORE_EXPORTED_FUNCTION(Omega::guid_t,OOCore_guid_t_create,0,());
inline Omega::guid_t Omega::guid_t::Create()
{
	return OOCore_guid_t_create();
}

#endif // OOCORE_TYPES_INL_INCLUDED_
