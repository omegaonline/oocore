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

// In order to 'export' a class from a DLL in an ABI agnostic way
// we export a whole set of extern "C" functions and call them in
// the member functions of the class.  Horrible I know!

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

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor1,2,((in),const char*,sz,(in),int,bUTF8));
Omega::string_t::string_t(const char* sz, bool bUTF8) :
	m_handle(0)
{
	if (sz)
		m_handle = static_cast<handle_t*>(OOCore_string_t__ctor1(sz,bUTF8 ? 1 : 0));

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
	OOCore_string_t__dctor(m_handle);
}

void Omega::string_t::release(handle_t* h)
{
	OOCore_string_t__dctor(h);
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_assign,2,((in),void*,h1,(in),const void*,h2));
Omega::string_t& Omega::string_t::operator = (const string_t& s)
{
	if (this != &s && (m_handle || s.m_handle))
	{
		m_handle = static_cast<handle_t*>(OOCore_string_t_assign(m_handle,s.m_handle));
		OMEGA_DEBUG_STASH_STRING();
	}
	return *this;
}

Omega::string_t& Omega::string_t::operator = (const wchar_t* sz)
{
	return operator =(string_t(sz));
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
		str = pszBuf;
		delete [] pszBuf;
	}
	else
		str = szBuf;

	return str;
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_add,2,((in),void*,h,(in),const void*,h2));
Omega::string_t& Omega::string_t::operator += (const string_t& s)
{
	m_handle = static_cast<handle_t*>(OOCore_string_t_add(m_handle,s.m_handle));
	OMEGA_DEBUG_STASH_STRING();
	return *this;
}

OOCORE_EXPORTED_FUNCTION(int,OOCore_string_t_cmp,2,((in),const void*,h1,(in),const void*,h2));
int Omega::string_t::Compare(const string_t& s) const
{
	return OOCore_string_t_cmp(m_handle,s.m_handle);
}

OOCORE_EXPORTED_FUNCTION(int,OOCore_string_t_cnc,2,((in),const void*,h1,(in),const void*,h2));
int Omega::string_t::CompareNoCase(const string_t& s) const
{
	return OOCore_string_t_cnc(m_handle,s.m_handle);
}

OOCORE_EXPORTED_FUNCTION(int,OOCore_string_t_isempty,1,((in),const void*,h));
bool Omega::string_t::IsEmpty() const
{
	return (OOCore_string_t_isempty(m_handle) != 0);
}

OOCORE_EXPORTED_FUNCTION(size_t,OOCore_string_t_len,1,((in),const void*,h));
size_t Omega::string_t::Length() const
{
	return OOCore_string_t_len(m_handle);
}

OOCORE_EXPORTED_FUNCTION(size_t,OOCore_string_t_find1,3,((in),const void*,h1,(in),const void*,h2,(in),size_t,s));
size_t Omega::string_t::Find(const string_t& str, size_t pos, bool bIgnoreCase) const
{
	if (!bIgnoreCase)
		return OOCore_string_t_find1(m_handle,str.m_handle,pos);
	else
		return this->ToLower().Find(str.ToLower(),pos,false);
}

OOCORE_EXPORTED_FUNCTION(size_t,OOCore_string_t_find2,4,((in),const void*,a,(in),wchar_t,b,(in),size_t,c,(in),int,d));
size_t Omega::string_t::Find(wchar_t c, size_t pos, bool bIgnoreCase) const
{
	if (!bIgnoreCase)
		return OOCore_string_t_find2(m_handle,c,pos,0);
	else
		return OOCore_string_t_find2(this->ToLower().m_handle,c,pos,1);
}

OOCORE_EXPORTED_FUNCTION(size_t,OOCore_string_t_rfind,4,((in),const void*,a,(in),wchar_t,b,(in),size_t,c,(in),int,d));
size_t Omega::string_t::ReverseFind(wchar_t c, size_t pos, bool bIgnoreCase) const
{
	if (!bIgnoreCase)
		return OOCore_string_t_rfind(m_handle,c,pos,0);
	else
		return OOCore_string_t_rfind(this->ToLower().m_handle,c,pos,1);
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_left,2,((in),const void*,a,(in),size_t,b));
Omega::string_t Omega::string_t::Left(size_t length) const
{
	return string_t(static_cast<handle_t*>(OOCore_string_t_left(m_handle,length)));
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_mid,3,((in),const void*,h,(in),size_t,a,(in),size_t,b));
Omega::string_t Omega::string_t::Mid(size_t start, size_t length) const
{
	return string_t(static_cast<handle_t*>(OOCore_string_t_mid(m_handle,start,length)));
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_right,2,((in),const void*,a,(in),size_t,b));
Omega::string_t Omega::string_t::Right(size_t length) const
{
	return string_t(static_cast<handle_t*>(OOCore_string_t_right(m_handle,length)));
}

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_string_t_clear,1,((in),void*,h));
Omega::string_t& Omega::string_t::Clear()
{
	OOCore_string_t_clear(m_handle);
	m_handle = 0;
	OMEGA_DEBUG_STASH_STRING();
	return *this;
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_tolower,1,((in),const void*,h));
Omega::string_t Omega::string_t::ToLower() const
{
	return string_t(static_cast<handle_t*>(OOCore_string_t_tolower(m_handle)));
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_toupper,1,((in),const void*,h));
Omega::string_t Omega::string_t::ToUpper() const
{
	return string_t(static_cast<handle_t*>(OOCore_string_t_toupper(m_handle)));
}

OOCORE_EXPORTED_FUNCTION(void*,OOCore_string_t_format,2,((in),const wchar_t*,sz,(in),va_list*,a));
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
	size_t pos = 0;
	while (Length() >= pos && (*this)[pos] == c)
		++pos;

	return Mid(pos);
}

Omega::string_t Omega::string_t::TrimLeft(const string_t& str) const
{
	size_t pos = 0;
	for (;Length() >= pos;++pos)
	{
		wchar_t c = (*this)[pos];

		const wchar_t* p = str.c_str();
		for (;*p!=c && *p!=L'\0';++p)
		{}

		if (*p != c)
			break;
	}
	return Mid(pos);
}

Omega::string_t Omega::string_t::TrimRight(wchar_t c) const
{
	size_t pos = Length();
	while (pos > 0 && (*this)[pos-1] == c)
		--pos;

	return Left(pos);
}

Omega::string_t Omega::string_t::TrimRight(const string_t& str) const
{
	size_t pos = Length();
	for (;pos > 0;--pos)
	{
		wchar_t c = (*this)[pos-1];

		const wchar_t* p = str.c_str();
		for (;*p!=c && *p!=L'\0';++p)
		{}

		if (*p != c)
			break;
	}
	return Left(pos);
}

inline Omega::string_t operator + (const Omega::string_t& lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

bool Omega::guid_t::operator==(const Omega::string_t& str) const
{
	return str.CompareNoCase(ToString()) == 0;
}

int Omega::guid_t::Compare(const guid_t& rhs) const
{
	// This is intentionally long-winded so we compare in an endianness-agnostic way.
	if (Data1 != rhs.Data1)
	{
		const byte_t* l = (const byte_t*)&Data1;
		const byte_t* r = (const byte_t*)&rhs.Data1;
		if (l[0] != r[0]) return (l[0] < r[0] ? -1 : 1);
		if (l[1] != r[1]) return (l[1] < r[1] ? -1 : 1);
		if (l[2] != r[2]) return (l[2] < r[2] ? -1 : 1);
		if (l[3] != r[3]) return (l[3] < r[3] ? -1 : 1);
	}
	else if (Data2 != rhs.Data2)
	{
		const byte_t* l = (const byte_t*)&Data2;
		const byte_t* r = (const byte_t*)&rhs.Data2;
		if (l[0] != r[0]) return (l[0] < r[0] ? -1 : 1);
		if (l[1] != r[1]) return (l[1] < r[1] ? -1 : 1);
	}
	else if (Data3 != rhs.Data3)
	{
		const byte_t* l = (const byte_t*)&Data3;
		const byte_t* r = (const byte_t*)&rhs.Data3;
		if (l[0] != r[0]) return (l[0] < r[0] ? -1 : 1);
		if (l[1] != r[1]) return (l[1] < r[1] ? -1 : 1);
	}
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

bool Omega::guid_t::operator==(const guid_t& rhs) const
{
	return Compare(rhs) == 0;
}

bool Omega::guid_t::operator!=(const Omega::guid_t& rhs) const
{
	return Compare(rhs) != 0;
}

bool Omega::guid_t::operator<(const guid_t& rhs) const
{
	return Compare(rhs) < 0;
}

bool Omega::guid_t::operator>(const guid_t& rhs) const
{
	return Compare(rhs) > 0;
}

OOCORE_EXPORTED_FUNCTION(Omega::string_t,OOCore_guid_t_to_string,1,((in),const Omega::guid_t&,guid));
Omega::string_t Omega::guid_t::ToString() const
{
	return OOCore_guid_t_to_string(*this);
}

OOCORE_EXPORTED_FUNCTION(int,OOCore_guid_t_from_string,2,((in),const wchar_t*,sz,(out),Omega::guid_t&,guid));
bool Omega::guid_t::FromString(const wchar_t* sz, Omega::guid_t& guid)
{
	return (OOCore_guid_t_from_string(sz,guid) != 0);
}

bool Omega::guid_t::FromString(const string_t& str, Omega::guid_t& guid)
{
	return (OOCore_guid_t_from_string(str.c_str(),guid) != 0);
}

Omega::guid_t Omega::guid_t::FromString(const wchar_t* sz)
{
	guid_t ret;
	if (!FromString(sz,ret))
		OMEGA_THROW(L"Invalid guid_t format string");
	return ret;
}

Omega::guid_t Omega::guid_t::FromString(const string_t& str)
{
	return FromString(str.c_str());
}

OOCORE_EXPORTED_FUNCTION(Omega::guid_t,OOCore_guid_t_create,0,());
Omega::guid_t Omega::guid_t::Create()
{
	return OOCore_guid_t_create();
}

#endif // OOCORE_TYPES_INL_INCLUDED_
