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

#include <string.h>

#if !defined(NDEBUG)
#define OMEGA_DEBUG_STASH_STRING()  m_debug_value = OOCore_string_t_cast(&m_handle,NULL)
#else
#define OMEGA_DEBUG_STASH_STRING()  (void)0
#endif

OOCORE_RAW_EXPORTED_FUNCTION(const char*,OOCore_string_t_cast,2,((in),const void*,h,(in),size_t*,plen));

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t__ctor,3,((in),void*,h1,(in),const char*,sz,(in),size_t,len));
inline Omega::string_t::string_t(const char* sz, size_t length)
{
	OOCore_string_t__ctor(&m_handle,sz,length);

	OMEGA_DEBUG_STASH_STRING();
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_addref,2,((in),void*,h1,(in),int,own));
inline void Omega::string_t::addref(handle_t& h, bool own)
{
	OOCore_string_t_addref(&h,own ? 1 : 0);
}

inline Omega::string_t::string_t(const handle_t& h, bool bAddref) :
		m_handle(h)
{
	if (bAddref)
		OOCore_string_t_addref(&m_handle,0);

	OMEGA_DEBUG_STASH_STRING();
}

inline Omega::string_t::string_t(const Omega::string_t& s) :
		m_handle(s.m_handle)
{
	OOCore_string_t_addref(&m_handle,0);

	OMEGA_DEBUG_STASH_STRING();
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_release,1,((in),void*,h));
inline void Omega::string_t::release(handle_t& h)
{
	OOCore_string_t_release(&h);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t__const_ctor,3,((in),void*,h1,(in),const char*,sz,(in),size_t,len));

template <size_t S>
inline Omega::string_t Omega::string_t::constant(const char (&arr)[S])
{
	handle_t h;
	OOCore_string_t__const_ctor(&h,arr,S-1);
	return string_t(h,false);
}

inline Omega::string_t Omega::string_t::constant(const char* sz, size_t len)
{
	handle_t h;
	OOCore_string_t__const_ctor(&h,sz,len);
	return string_t(h,false);
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
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_assign,2,((in),void*,h1,(in),const void*,h2));
inline Omega::string_t& Omega::string_t::operator = (const string_t& s)
{
	if (this != &s)
	{
		OOCore_string_t_assign(&m_handle,&s.m_handle);
		OMEGA_DEBUG_STASH_STRING();
	}
	return *this;
}

inline const char* Omega::string_t::c_str() const
{
	return OOCore_string_t_cast(&m_handle,NULL);
}

inline const char Omega::string_t::operator[](size_t i) const
{
	size_t len = 0;
	const char* buf = OOCore_string_t_cast(&m_handle,&len);

	if (i >= len)
		return '\0';

	return buf[i];
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_append1,2,((in),void*,h,(in),const void*,h2));
inline Omega::string_t& Omega::string_t::operator += (const string_t& s)
{
	OOCore_string_t_append1(&m_handle,&s.m_handle);
	OMEGA_DEBUG_STASH_STRING();
	return *this;
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_append2,3,((in),void*,h,(in),const char*,sz,(in),size_t,len));
inline Omega::string_t& Omega::string_t::operator += (char c)
{
	OOCore_string_t_append2(&m_handle,&c,1);
	OMEGA_DEBUG_STASH_STRING();
	return *this;
}

inline Omega::string_t& Omega::string_t::operator += (const char* sz)
{
	OOCore_string_t_append2(&m_handle,sz,string_t::npos);
	OMEGA_DEBUG_STASH_STRING();
	return *this;
}

OOCORE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cmp1,4,((in),const void*,h1,(in),const void*,h2,(in),size_t,pos,(in),size_t,length));
inline int Omega::string_t::Compare(const string_t& s, size_t pos, size_t length) const
{
	return OOCore_string_t_cmp1(&m_handle,&s.m_handle,pos,length);
}

OOCORE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cmp2,2,((in),const void*,h1,(in),const char*,sz));
inline int Omega::string_t::Compare(const char* sz) const
{
	return OOCore_string_t_cmp2(&m_handle,sz);
}

inline bool Omega::string_t::IsEmpty() const
{
	return (Length() == 0);
}

inline bool Omega::string_t::operator !() const
{
	return (Length() == 0);
}

inline size_t Omega::string_t::Length() const
{
	size_t len = 0;
	OOCore_string_t_cast(&m_handle,&len);
	return len;
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find1,3,((in),const void*,s1,(in),char,c,(in),size_t,pos));
inline size_t Omega::string_t::Find(char c, size_t pos) const
{
	return OOCore_string_t_find1(&m_handle,c,pos);
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_not,3,((in),const void*,a,(in),char,b,(in),size_t,c));
inline size_t Omega::string_t::FindNot(char c, size_t pos) const
{
	return OOCore_string_t_find_not(&m_handle,c,pos);
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find2,3,((in),const void*,h1,(in),const void*,h2,(in),size_t,s));
inline size_t Omega::string_t::Find(const string_t& str, size_t pos) const
{
	return OOCore_string_t_find2(&m_handle,&str.m_handle,pos);
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_oneof,3,((in),const void*,h1,(in),const void*,h2,(in),size_t,s));
inline size_t Omega::string_t::FindOneOf(const string_t& str, size_t pos) const
{
	return OOCore_string_t_find_oneof(&m_handle,&str.m_handle,pos);
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_notof,3,((in),const void*,h1,(in),const void*,h2,(in),size_t,s));
inline size_t Omega::string_t::FindNotOf(const string_t& str, size_t pos) const
{
	return OOCore_string_t_find_notof(&m_handle,&str.m_handle,pos);
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_rfind,3,((in),const void*,a,(in),char,b,(in),size_t,c));
inline size_t Omega::string_t::ReverseFind(char c, size_t pos) const
{
	return OOCore_string_t_rfind(&m_handle,c,pos);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_left,4,((in),void*,h1,(in),const void*,h2,(in),size_t,a,(in),size_t,b));
inline Omega::string_t Omega::string_t::Left(size_t length) const
{
	handle_t h;
	OOCore_string_t_left(&h,&m_handle,0,length);
	return string_t(h,false);
}

inline Omega::string_t Omega::string_t::Mid(size_t start, size_t length) const
{
	handle_t h;
	OOCore_string_t_left(&h,&m_handle,start,length);
	return string_t(h,false);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_right,3,((in),void*,h1,(in),const void*,h2,(in),size_t,b));
inline Omega::string_t Omega::string_t::Right(size_t length) const
{
	handle_t h;
	OOCore_string_t_right(&h,&m_handle,length);
	return string_t(h,false);
}

inline Omega::string_t& Omega::string_t::Clear()
{
	OOCore_string_t_release(&m_handle);
	OMEGA_DEBUG_STASH_STRING();
	return *this;
}

inline Omega::string_t Omega::string_t::TrimLeft(const string_t& str) const
{
	return Mid(FindNotOf(str));
}

inline Omega::string_t Omega::string_t::TrimRight(const string_t& str) const
{
	const char* s = c_str();
	const char* p = s + Length()-1;
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

inline Omega::string_t operator + (const char* lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t::constant(lhs,strlen(lhs)) += rhs);
}

inline Omega::string_t operator + (const Omega::string_t& lhs, const char* rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

inline Omega::string_t operator + (char lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t::constant(&lhs,1) += rhs);
}

inline Omega::string_t operator + (const Omega::string_t& lhs, char rhs)
{
	return (Omega::string_t(lhs) += rhs);
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
	return val;
}

inline Omega::string_t Omega::Formatting::ToString(const char* val, const Omega::string_t& strFormat)
{
	return string_t(val);
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

OOCORE_RAW_EXPORTED_FUNCTION(int,OOCore_guid_t_from_string,3,((in),const char*,sz,(in),int,throws,(in_out),Omega::guid_base_t*,result));
inline bool Omega::guid_t::FromString(const char* sz, Omega::guid_t& guid)
{
	return (OOCore_guid_t_from_string(sz,0,&guid) != 0);
}

inline bool Omega::guid_t::FromString(const string_t& str, Omega::guid_t& guid)
{
	return (str.Length() == 38 && FromString(str.c_str(),guid));
}

inline Omega::guid_t::guid_t(const char* sz)
{
	OOCore_guid_t_from_string(sz,1,this);
}

inline Omega::guid_t::guid_t(const string_t& str)
{
	OOCore_guid_t_from_string(str.Length() == 38 ? str.c_str() : "\0",1,this);
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

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_formatter_t_get_arg,3,((in),const void*,handle,(out),unsigned long&,index,(out),Omega::string_t&,fmt));
OOCORE_EXPORTED_FUNCTION_VOID(OOCore_formatter_t_set_arg,3,((in),void*,handle,(in),unsigned long,index,(in),const Omega::string_t&,arg));

template <typename T>
inline Omega::Formatting::formatter_t& Omega::Formatting::formatter_t::operator % (const T& rhs)
{
	unsigned long index;
	string_t strFormat;

	OOCore_formatter_t_get_arg(m_handle,index,strFormat);
	OOCore_formatter_t_set_arg(m_handle,index,Formatting::ToString(rhs,strFormat));

	return *this;
}

OOCORE_EXPORTED_FUNCTION(Omega::string_t,OOCore_formatter_t_cast,1,((in),const void*,handle));
inline Omega::Formatting::formatter_t::operator Omega::string_t() const
{
	return OOCore_formatter_t_cast(m_handle);
}

template <typename T>
inline Omega::Formatting::formatter_t operator % (const Omega::string_t& lhs, T rhs)
{
	return Omega::Formatting::formatter_t(lhs) % rhs;
}

inline Omega::Formatting::formatter_t operator % (const char* lhs, const Omega::string_t& rhs)
{
	return Omega::Formatting::formatter_t(Omega::string_t(lhs)) % rhs;
}

#endif // OOCORE_TYPES_INL_INCLUDED_
