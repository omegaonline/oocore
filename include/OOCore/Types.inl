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

#ifdef OMEGA_DEBUG
#define OMEGA_DEBUG_STASH_STRING()	m_debug_value = OOCore_string_t_cast(m_handle)
#else
#define OMEGA_DEBUG_STASH_STRING()	(void)0
#endif

OOCORE_EXPORTED_FUNCTION(const wchar_t*,OOCore_string_t_cast,1,((in),const void*,h));

Omega::string_t::string_t(handle_t* h) :
	m_handle(h)
{
	OMEGA_DEBUG_STASH_STRING();
}

Omega::string_t::string_t() : m_handle(0)
{
	OMEGA_DEBUG_STASH_STRING();
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor1,3,((in),const char*,sz,(in),size_t,len,(in),int,bUTF8));
Omega::string_t::string_t(const char* sz, bool bUTF8, size_t length) :
	m_handle(0)
{
	if (sz)
		m_handle = static_cast<handle_t*>(OOCore_string_t__ctor1(sz,length,bUTF8 ? 1 : 0));

	OMEGA_DEBUG_STASH_STRING();
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor2,1,((in),const void*,s1));
Omega::string_t::string_t(const Omega::string_t& s) :
	m_handle(0)
{
	if (s.m_handle)
		m_handle = static_cast<handle_t*>(OOCore_string_t__ctor2(s.m_handle));

	OMEGA_DEBUG_STASH_STRING();
}

void Omega::string_t::addref(handle_t* h)
{
	if (h)
		OOCore_string_t__ctor2(h);
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor3,2,((in),const wchar_t*,wsz,(in),size_t,length));
Omega::string_t::string_t(const wchar_t* wsz, size_t length) :
	m_handle(0)
{
	if (wsz)
		m_handle = static_cast<handle_t*>(OOCore_string_t__ctor3(wsz,length));

	OMEGA_DEBUG_STASH_STRING();
}

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_string_t__dctor,1,((in),void*,h));
Omega::string_t::~string_t()
{
	if (m_handle)
		OOCore_string_t__dctor(m_handle);
}

void Omega::string_t::release(handle_t* h)
{
	if (h)
		OOCore_string_t__dctor(h);
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_assign1,2,((in),void*,h1,(in),const void*,h2));
Omega::string_t& Omega::string_t::operator = (const string_t& s)
{
	if (this != &s && m_handle != s.m_handle)
	{
		m_handle = static_cast<handle_t*>(OOCore_string_t_assign1(m_handle,s.m_handle));
		OMEGA_DEBUG_STASH_STRING();
	}
	return *this;
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_assign2,2,((in),void*,h1,(in),const wchar_t*,wsz));
Omega::string_t& Omega::string_t::operator = (const wchar_t* wsz)
{
	m_handle = static_cast<handle_t*>(OOCore_string_t_assign2(m_handle,wsz));
	OMEGA_DEBUG_STASH_STRING();
	return *this;
}

const wchar_t* Omega::string_t::c_str() const
{
	return OOCore_string_t_cast(m_handle);
}

OOCORE_EXPORTED_FUNCTION(size_t,OOCore_string_t_toutf8,3,((in),const void*,h,(in),char*,sz,(in),size_t,size));
size_t Omega::string_t::ToUTF8(char* sz, size_t size) const
{
	return OOCore_string_t_toutf8(m_handle,sz,size);
}

inline std::string Omega::string_t::ToUTF8() const
{
	std::string str;
	char szBuf[128];
	size_t len = OOCore_string_t_toutf8(m_handle,szBuf,128);
	if (len > 128)
	{
		char* pszBuf = 0;
		OMEGA_NEW(pszBuf,char[len]);
		try
		{
			OOCore_string_t_toutf8(m_handle,pszBuf,len);
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

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_add,2,((in),void*,h,(in),const void*,h2));
Omega::string_t& Omega::string_t::operator += (const string_t& s)
{
	if (s.m_handle)
	{
		m_handle = static_cast<handle_t*>(OOCore_string_t_add(m_handle,s.m_handle));
		OMEGA_DEBUG_STASH_STRING();
	}
	return *this;
}

OOCORE_EXPORTED_FUNCTION(int,OOCore_string_t_cmp1,5,((in),const void*,h1,(in),const void*,h2,(in),size_t,pos,(in),size_t,length,(in),int,bIgnoreCase));
int Omega::string_t::Compare(const string_t& s, size_t pos, size_t length, bool bIgnoreCase) const
{
	if (m_handle == s.m_handle)
		return 0;

	if (!m_handle)
		return -1;

	if (!s.m_handle)
		return 1;

	return OOCore_string_t_cmp1(m_handle,s.m_handle,pos,length,(bIgnoreCase ? 1 : 0));
}

OOCORE_EXPORTED_FUNCTION(int,OOCore_string_t_cmp2,5,((in),const void*,h,(in),const wchar_t*,wsz,(in),size_t,pos,(in),size_t,length,(in),int,bIgnoreCase));
int Omega::string_t::Compare(const wchar_t* wsz, size_t pos, size_t length, bool bIgnoreCase) const
{
	if (!m_handle)
	{
		if (!wsz)
			return 0;

		if (wcslen(wsz) == 0)
			return 0;
		
		return -1;
	}

	if (!wsz)
		return 1;

	return OOCore_string_t_cmp2(m_handle,wsz,pos,length,(bIgnoreCase ? 1 : 0));
}

bool Omega::string_t::IsEmpty() const
{
	return (m_handle == 0);
}

OOCORE_EXPORTED_FUNCTION(size_t,OOCore_string_t_len,1,((in),const void*,h));
size_t Omega::string_t::Length() const
{
	if (!m_handle)
		return 0;

	return OOCore_string_t_len(m_handle);
}

OOCORE_EXPORTED_FUNCTION(size_t,OOCore_string_t_find1,4,((in),const void*,a,(in),wchar_t,b,(in),size_t,c,(in),int,d));
size_t Omega::string_t::Find(wchar_t c, size_t pos, bool bIgnoreCase) const
{
	if (!m_handle)
		return string_t::npos;

	return OOCore_string_t_find1(m_handle,c,pos,(bIgnoreCase ? 1 : 0));
}

OOCORE_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_not,4,((in),const void*,a,(in),wchar_t,b,(in),size_t,c,(in),int,d));
size_t Omega::string_t::FindNot(wchar_t c, size_t pos, bool bIgnoreCase) const
{
	if (!m_handle)
		return string_t::npos;

	return OOCore_string_t_find_not(m_handle,c,pos,(bIgnoreCase ? 1 : 0));
}

OOCORE_EXPORTED_FUNCTION(size_t,OOCore_string_t_find2,4,((in),const void*,h1,(in),const void*,h2,(in),size_t,s,(in),int,bIgnoreCase));
size_t Omega::string_t::Find(const string_t& str, size_t pos, bool bIgnoreCase) const
{
	if (!m_handle || !str.m_handle)
		return string_t::npos;

	return OOCore_string_t_find2(m_handle,str.m_handle,pos,(bIgnoreCase ? 1 : 0));
}

OOCORE_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_oneof,4,((in),const void*,h1,(in),const void*,h2,(in),size_t,s,(in),int,bIgnoreCase));
size_t Omega::string_t::FindOneOf(const string_t& str, size_t pos, bool bIgnoreCase) const
{
	if (!m_handle || !str.m_handle)
		return string_t::npos;

	return OOCore_string_t_find_oneof(m_handle,str.m_handle,pos,(bIgnoreCase ? 1 : 0));
}

OOCORE_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_notof,4,((in),const void*,h1,(in),const void*,h2,(in),size_t,s,(in),int,bIgnoreCase));
size_t Omega::string_t::FindNotOf(const string_t& str, size_t pos, bool bIgnoreCase) const
{
	if (!m_handle || !str.m_handle)
		return string_t::npos;

	return OOCore_string_t_find_notof(m_handle,str.m_handle,pos,(bIgnoreCase ? 1 : 0));
}

OOCORE_EXPORTED_FUNCTION(size_t,OOCore_string_t_rfind,4,((in),const void*,a,(in),wchar_t,b,(in),size_t,c,(in),int,d));
size_t Omega::string_t::ReverseFind(wchar_t c, size_t pos, bool bIgnoreCase) const
{
	if (!m_handle)
		return string_t::npos;

	return OOCore_string_t_rfind(m_handle,c,pos,(bIgnoreCase ? 1 : 0));
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_left,2,((in),const void*,a,(in),size_t,b));
Omega::string_t Omega::string_t::Left(size_t length) const
{
	if (length == 0 || !m_handle)
		return string_t((handle_t*)0);

	return string_t(static_cast<handle_t*>(OOCore_string_t_left(m_handle,length)));
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_mid,3,((in),const void*,h,(in),size_t,a,(in),size_t,b));
Omega::string_t Omega::string_t::Mid(size_t start, size_t length) const
{
	if (length == 0 || !m_handle)
		return string_t((handle_t*)0);

	return string_t(static_cast<handle_t*>(OOCore_string_t_mid(m_handle,start,length)));
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_right,2,((in),const void*,a,(in),size_t,b));
Omega::string_t Omega::string_t::Right(size_t length) const
{
	if (length == 0 || !m_handle)
		return string_t((handle_t*)0);

	return string_t(static_cast<handle_t*>(OOCore_string_t_right(m_handle,length)));
}

Omega::string_t& Omega::string_t::Clear()
{
	if (m_handle)
	{
		OOCore_string_t__dctor(m_handle);
		m_handle = 0;
		OMEGA_DEBUG_STASH_STRING();
	}
	return *this;
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_tolower,1,((in),const void*,h));
Omega::string_t Omega::string_t::ToLower() const
{
	if (!m_handle)
		return string_t((handle_t*)0);

	return string_t(static_cast<handle_t*>(OOCore_string_t_tolower(m_handle)));
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_toupper,1,((in),const void*,h));
Omega::string_t Omega::string_t::ToUpper() const
{
	if (!m_handle)
		return string_t((handle_t*)0);

	return string_t(static_cast<handle_t*>(OOCore_string_t_toupper(m_handle)));
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_format,2,((in),const wchar_t*,wsz,(in),va_list*,a));
Omega::string_t Omega::string_t::Format(const wchar_t* pszFormat, ...)
{
	va_list list;
	va_start(list,pszFormat);

	handle_t* h2 = static_cast<handle_t*>(OOCore_string_t_format(pszFormat,&list));

	va_end(list);

	if (h2)
		return string_t(h2);
	else
		return string_t();
}

Omega::string_t Omega::string_t::TrimLeft(wchar_t c) const
{
	return Mid(FindNot(c));
}

Omega::string_t Omega::string_t::TrimLeft(const string_t& str) const
{
	return Mid(FindNotOf(str));
}

Omega::string_t Omega::string_t::TrimRight(wchar_t c) const
{
	const wchar_t* s = c_str();
	const wchar_t* p = s + Length()-1;
	for (;*p == c && p>=s;--p)
		;

	return Left(p+1-s);
}

Omega::string_t Omega::string_t::TrimRight(const string_t& str) const
{
	const wchar_t* s = c_str();
	const wchar_t* p = s + Length()-1;
	for (;str.Find(*p) != string_t::npos && p>=s;--p)
		;

	return Left(p+1-s);
}

inline Omega::string_t operator + (const Omega::string_t& lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

int Omega::guid_t::Compare(const guid_t& rhs) const
{
	if (Data1 != rhs.Data1)
		return (Data1 < rhs.Data1 ? -1 : 1);
	else if (Data2 != rhs.Data2)
		return (Data2 < rhs.Data2 ? -1 : 1);
	else if (Data3 != rhs.Data3)
		return (Data3 < rhs.Data3 ? -1 : 1);
	else if (*reinterpret_cast<const uint64_t*>(Data4) != *reinterpret_cast<const uint64_t*>(rhs.Data4))
	{
		for (int i=0;i<8;++i)
		{
			if (Data4[i] != rhs.Data4[i])
				return (Data4[i] < rhs.Data4[i] ? -1 : 1);
		}
	}

	return 0;
}

OOCORE_EXPORTED_FUNCTION(Omega::string_t,OOCore_guid_t_to_string,1,((in),const Omega::guid_t&,guid));
Omega::string_t Omega::guid_t::ToString() const
{
	return OOCore_guid_t_to_string(*this);
}

OOCORE_EXPORTED_FUNCTION(int,OOCore_guid_t_from_string,2,((in),const wchar_t*,wsz,(out),Omega::guid_t&,guid));
bool Omega::guid_t::FromString(const string_t& str, Omega::guid_t& guid)
{
	return (OOCore_guid_t_from_string(str.c_str(),guid) != 0);
}

Omega::guid_t Omega::guid_t::FromString(const string_t& str)
{
	guid_t ret;
	if (!FromString(str,ret))
		OMEGA_THROW(L"Invalid guid_t format string");
	return ret;
}

OOCORE_EXPORTED_FUNCTION(Omega::guid_t,OOCore_guid_t_create,0,());
Omega::guid_t Omega::guid_t::Create()
{
	return OOCore_guid_t_create();
}

#endif // OOCORE_TYPES_INL_INCLUDED_
