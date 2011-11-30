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

#if !defined(NDEBUG)
#define OMEGA_DEBUG_STASH_STRING()  m_debug_value = (m_handle ? OOCore_string_t_cast_w(m_handle) : L"")
#else
#define OMEGA_DEBUG_STASH_STRING()  (void)0
#endif

OOCORE_RAW_EXPORTED_FUNCTION(const wchar_t*,OOCore_string_t_cast_w,1,((in),const void*,h));

inline Omega::string_t::string_t(handle_t* h, bool bAddref) :
		m_handle(bAddref ? addref(h,false) : h)
{
	OMEGA_DEBUG_STASH_STRING();
}

inline Omega::string_t::string_t() : m_handle(NULL)
{
	OMEGA_DEBUG_STASH_STRING();
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor1,3,((in),const char*,sz,(in),size_t,len,(in),int,bUTF8));
inline Omega::string_t::string_t(const char* sz, bool bUTF8, size_t length) :
		m_handle(NULL)
{
	if (sz)
		m_handle = static_cast<handle_t*>(OOCore_string_t__ctor1(sz,length,bUTF8 ? 1 : 0));

	OMEGA_DEBUG_STASH_STRING();
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_addref,2,((in),void*,s1,(in),int,own));
inline Omega::string_t::handle_t* Omega::string_t::addref(handle_t* h, bool own)
{
	if (!h)
		return NULL;

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
		m_handle(NULL)
{
	m_handle = static_cast<handle_t*>(OOCore_string_t__ctor2(arr,S-1,copy ? 1 : 0));

	OMEGA_DEBUG_STASH_STRING();
}

inline Omega::string_t::string_t(const wchar_t (&)[1]) :
		m_handle(NULL)
{
	OMEGA_DEBUG_STASH_STRING();
}

inline Omega::string_t::string_t(const wchar_t* wsz, size_t length, bool copy) :
		m_handle(NULL)
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
	try
	{
		release(m_handle);
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}
	catch (...)
	{}
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

inline const wchar_t* Omega::string_t::c_wstr() const
{
	if (!m_handle)
		return L"";

	return OOCore_string_t_cast_w(m_handle);
}

OOCORE_RAW_EXPORTED_FUNCTION(const char*,OOCore_string_t_to_char,3,((in),const void*,h,(in),int,utf8,(out),size_t*,len));
inline const char* Omega::string_t::c_ustr(size_t* len) const
{
	return OOCore_string_t_to_char(m_handle,1,len);
}

inline const char* Omega::string_t::c_nstr(size_t* len) const
{
	return OOCore_string_t_to_char(m_handle,0,len);
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
		return string_t((handle_t*)NULL,false);

	return string_t(static_cast<handle_t*>(OOCore_string_t_left(m_handle,length)),false);
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_mid,3,((in),const void*,h,(in),size_t,a,(in),size_t,b));
inline Omega::string_t Omega::string_t::Mid(size_t start, size_t length) const
{
	if (length == 0 || !m_handle)
		return string_t((handle_t*)NULL,false);

	return string_t(static_cast<handle_t*>(OOCore_string_t_mid(m_handle,start,length)),false);
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_right,2,((in),const void*,a,(in),size_t,b));
inline Omega::string_t Omega::string_t::Right(size_t length) const
{
	if (length == 0 || !m_handle)
		return string_t((handle_t*)NULL,false);

	return string_t(static_cast<handle_t*>(OOCore_string_t_right(m_handle,length)),false);
}

inline Omega::string_t& Omega::string_t::Clear()
{
	release(m_handle);
	m_handle = NULL;

	OMEGA_DEBUG_STASH_STRING();

	return *this;
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
	const wchar_t* s = c_wstr();
	const wchar_t* p = s + Length()-1;
	while (p>=s && *p == c)
		--p;

	return Left(p+1-s);
}

inline Omega::string_t Omega::string_t::TrimRight(const string_t& str) const
{
	const wchar_t* s = c_wstr();
	const wchar_t* p = s + Length()-1;
	while (p>=s && str.Find(*p) != string_t::npos)
		--p;

	return Left(p+1-s);
}

template <typename T>
inline Omega::string_t& Omega::string_t::operator %= (T rhs)
{
	return (*this = (*this % rhs));
}

inline Omega::string_t operator + (const Omega::string_t& lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

inline Omega::string_t operator + (const wchar_t* lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t(lhs,Omega::string_t::npos) += rhs);
}

inline Omega::string_t operator + (const Omega::string_t& lhs, const wchar_t* rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

inline Omega::string_t operator + (wchar_t lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t(&lhs,1) += rhs);
}

inline Omega::string_t operator + (const Omega::string_t& lhs, wchar_t rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

template <typename T>
inline Omega::Formatting::formatter_t operator % (const Omega::string_t& lhs, T rhs)
{
	return Omega::Formatting::formatter_t(lhs) % rhs;
}

inline Omega::Formatting::formatter_t operator % (const wchar_t* lhs, const Omega::string_t& rhs)
{
	return Omega::Formatting::formatter_t(Omega::string_t(lhs,Omega::string_t::npos)) % rhs;
}

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
			struct general_formatter_t
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
			struct general_formatter_t<const T>
			{
				typedef typename general_formatter_t<T>::type type;
			};

			template <typename T>
			struct general_formatter_t<T&>
			{
				typedef typename general_formatter_t<T>::type type;
			};

			template <typename T, size_t S>
			struct general_formatter_t<T[S]>
			{
				typedef typename general_formatter_t<T*>::type type;
			};

			template <typename T>
			struct general_formatter_t<const T*>
			{
				typedef typename general_formatter_t<T*>::type type;
			};

			// Don't pass pointers
			template <typename T> struct general_formatter_t<T*>;

			// Long doubles are not compiler agnostic...
			template <> struct general_formatter_t<long double>;
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
	return System::Internal::general_formatter_t<T>::type::ToString(val,strFormat);
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

OOCORE_RAW_EXPORTED_FUNCTION(int,OOCore_guid_t_from_string,2,((in),const wchar_t*,sz,(in_out),Omega::guid_base_t*,result));
inline bool Omega::guid_t::FromString(const wchar_t* wsz, Omega::guid_t& guid)
{
	return (OOCore_guid_t_from_string(wsz,&guid) != 0);
}

inline bool Omega::guid_t::FromString(const string_t& str, Omega::guid_t& guid)
{
	return (str.Length() == 38 && FromString(str.c_wstr(),guid));
}

inline Omega::guid_t::guid_t(const wchar_t* sz)
{
	if (!guid_t::FromString(sz,*this))
		throw Formatting::IFormattingException::Create(string_t(L"{0} is not an Omega::guid_t string representation") % sz);
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

///////////////////////////////////////////////////////////////
// Formatting starts here

OOCORE_EXPORTED_FUNCTION(void*,OOCore_formatter_t__ctor1,1,((in),const Omega::string_t&,format));
inline Omega::Formatting::formatter_t::formatter_t(const string_t& format) : m_handle(static_cast<handle_t*>(OOCore_formatter_t__ctor1(format)))
{
}

inline Omega::Formatting::formatter_t::~formatter_t()
{
	try
	{
		free_handle(m_handle);
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}
	catch (...)
	{}
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_formatter_t__ctor2,1,((in),const void*,handle));
inline Omega::Formatting::formatter_t::handle_t* Omega::Formatting::formatter_t::clone_handle(const formatter_t& rhs)
{
	return static_cast<handle_t*>(OOCore_formatter_t__ctor2(rhs.m_handle));
}

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_formatter_t__dctor,1,((in),void*,handle));
inline void Omega::Formatting::formatter_t::free_handle(handle_t* h)
{
	OOCore_formatter_t__dctor(h);
}

OOCORE_EXPORTED_FUNCTION(int,OOCore_formatter_t_get_arg,3,((in),const void*,handle,(out),unsigned long&,index,(out),Omega::string_t&,fmt));
OOCORE_EXPORTED_FUNCTION_VOID(OOCore_formatter_t_set_arg,3,((in),void*,handle,(in),unsigned long,index,(in),const Omega::string_t&,arg));

template <typename T>
inline Omega::Formatting::formatter_t& Omega::Formatting::formatter_t::operator % (const T& rhs)
{
	unsigned long index;
	string_t strFormat;
	if (!OOCore_formatter_t_get_arg(m_handle,index,strFormat))
		throw Formatting::IFormattingException::Create(L"No more formatting arguments");

	OOCore_formatter_t_set_arg(m_handle,index,Formatting::ToString(rhs,strFormat));

	return *this;
}

OOCORE_EXPORTED_FUNCTION(Omega::string_t,OOCore_formatter_t_cast,1,((in),const void*,handle));
inline Omega::Formatting::formatter_t::operator Omega::string_t() const
{
	return OOCore_formatter_t_cast(m_handle);
}

#endif // OOCORE_TYPES_INL_INCLUDED_
