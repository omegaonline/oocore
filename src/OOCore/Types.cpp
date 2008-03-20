///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
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

#include "OOCore_precomp.h"

namespace OOCore
{
	ACE_WString from_utf8(const char* sz);
	ACE_CString to_utf8(const wchar_t* wsz);

	struct StringNode
	{
		StringNode() : m_refcount(1)
		{}

		StringNode(const char* sz) : m_str(from_utf8(sz)), m_refcount(1)
		{}

		StringNode(const wchar_t* sz) : m_str(sz), m_refcount(1)
		{}

		StringNode(const wchar_t* sz, size_t length) : m_str(sz,length), m_refcount(1)
		{}

		StringNode(const ACE_WString& s) : m_str(s), m_refcount(1)
		{}

		void AddRef()
		{
			++m_refcount;
		}

		void Release()
		{
			if (--m_refcount==0)
				delete this;
		}

		ACE_WString	m_str;

	private:
		ACE_Atomic_Op<ACE_Thread_Mutex,Omega::uint32_t> m_refcount;
	};
}

using namespace OOCore;

ACE_WString OOCore::from_utf8(const char* sz)
{
    if (!sz || *sz=='\0')
        return L"";

	static const int trailingBytesForUTF8[256] =
	{
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 0x00 - 0x1F
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 0x20 - 0x3F
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 0x40 - 0x5F
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 0x60 - 0x7F
		9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,	// 0x80 - 0x9F
		9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,	// 0xA0 - 0xBF
		9,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	// 0xC0 - 0xDF
		2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,11,11,11,4,4,4,4,5,5,6,7	// 0xE0 - 0xFF
	};

	ACE_WString strRet;
	strRet.fast_resize(ACE_OS::strlen(sz));
	for (const char* p=sz;*p!='\0';)
	{
		unsigned int c;
		unsigned char v = *p++;
		int trailers = trailingBytesForUTF8[v];
		switch (trailers)
		{
		case 0:
			c = v;
			break;

		case 1:
			c = v & 0x1f;
			break;

		case 2:
			c = v & 0xf;
			break;

		case 3:
			c = v & 0x7;
			break;

		default:
			trailers &= 7;
			c = L'\xFFFD';
		}

		for (int i=0;i<trailers;++i,++p)
		{
			if (*p == '\0')
			{
				c = L'\xFFFD';
				break;
			}

			if (c != L'\xFFFD')
			{
				c <<= 6;
				c += (*p & 0x3f);
			}
		}

		if (sizeof(wchar_t)==2)
		{
			if (c & 0xFFFF0000)
			{
				// Oh god.. we're big!
				c -= 0x10000;

#if (OMEGA_BYTE_ORDER == OMEGA_BIG_ENDIAN)
				strRet += static_cast<wchar_t>((c >> 10) | 0xD800);
				strRet += static_cast<wchar_t>((c & 0x3ff) | 0xDC00);
#else
				strRet += static_cast<wchar_t>((c & 0x3ff) | 0xDC00);
				strRet += static_cast<wchar_t>((c >> 10) | 0xD800);
#endif
				continue;
			}
		}

		strRet += static_cast<wchar_t>(c);
	}

	return strRet;
}

ACE_CString OOCore::to_utf8(const wchar_t* wsz)
{
    if (!wsz || *wsz==L'\0')
        return "";

	ACE_CString strRet;
	strRet.fast_resize(ACE_OS::strlen(wsz)*2);
	for (const wchar_t* p=wsz;*p!=0;)
	{
		char c;
		unsigned int v = *p++;

		if (v <= 0x7f)
			strRet += static_cast<char>(v);
		else if (v <= 0x7FF)
		{
			c = static_cast<char>(v >> 6) | 0xc0;
			strRet += c;
			c = static_cast<char>(v & 0x3f) | 0x80;
			strRet += c;
		}
		else if (v <= 0xFFFF)
		{
			// Invalid range
			if (v > 0xD7FF && v < 0xE00)
				strRet += "\xEF\xBF\xBD";
			else
			{
				c = static_cast<char>(v >> 12) | 0xe0;
				strRet += c;
				c = static_cast<char>((v & 0xfc0) >> 6) | 0x80;
				strRet += c;
				c = static_cast<char>(v & 0x3f) | 0x80;
				strRet += c;
			}
		}
		else if (v <= 0x10FFFF)
		{
			c = static_cast<char>(v >> 18) | 0xf0;
			strRet += c;
			c = static_cast<char>((v & 0x3f000) >> 12) | 0x80;
			strRet += c;
			c = static_cast<char>((v & 0xfc0) >> 6) | 0x80;
			strRet += c;
			c = static_cast<char>(v & 0x3f) | 0x80;
			strRet += c;
		}
		else
			strRet += "\xEF\xBF\xBD";
	}

	return strRet;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t__ctor1,0,())
{
	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode());
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t__ctor2,2,((in),const char*,sz,(in),Omega::bool_t,bUTF8))
{
	StringNode* pNode;
	if (bUTF8)
		OMEGA_NEW(pNode,StringNode(sz));
	else
		OMEGA_NEW(pNode,StringNode(ACE_Ascii_To_Wide(sz).wchar_rep()));
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t__ctor3,1,((in),const void*,s1))
{
	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(static_cast<const StringNode*>(s1)->m_str));
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t__ctor4,2,((in),const wchar_t*,wsz,(in),size_t,length))
{
	StringNode* pNode;
	if (length == Omega::string_t::npos)
		OMEGA_NEW(pNode,StringNode(wsz));
	else
		OMEGA_NEW(pNode,StringNode(wsz,length));
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(string_t__dctor,1,((in),void*,s1))
{
	static_cast<StringNode*>(s1)->Release();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_assign_1,2,((in),void*,s1,(in),const void*,s2))
{
	static_cast<StringNode*>(s1)->Release();

	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(static_cast<const StringNode*>(s2)->m_str));
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_assign_2,2,((in),void*,s1,(in),const char*,sz))
{
	static_cast<StringNode*>(s1)->Release();

	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(ACE_Ascii_To_Wide(sz).wchar_rep()));
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_assign_3,2,((in),void*,s1,(in),const wchar_t*,wsz))
{
	static_cast<StringNode*>(s1)->Release();

	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(wsz));
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(const wchar_t*,string_t_cast,1,((in),const void*,s1))
{
	return static_cast<const StringNode*>(s1)->m_str.c_str();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_toutf8,3,((in),const void*,h,(in),char*,sz,(in),size_t,size))
{
	ACE_CString str = to_utf8(static_cast<const StringNode*>(h)->m_str.c_str());
	if (size < str.length()+1)
	{
		ACE_OS::strncpy(sz,str.c_str(),size-1);
		sz[size-1] = '\0';
	}
	else
	{
		ACE_OS::strcpy(sz,str.c_str());
	}
	return str.length() + 1;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_add1,2,((in),void*,s1,(in),const void*,s2))
{
	StringNode* pOld = static_cast<StringNode*>(s1);

	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(pOld->m_str));

	pOld->Release();

	pNode->m_str += static_cast<const StringNode*>(s2)->m_str;
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_add2,2,((in),void*,s1,(in),const char*,sz))
{
	StringNode* pOld = static_cast<StringNode*>(s1);

	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(pOld->m_str));

	pOld->Release();

	pNode->m_str += ACE_Ascii_To_Wide(sz).wchar_rep();
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_add3,2,((in),void*,s1,(in),const wchar_t*,wsz))
{
	StringNode* pOld = static_cast<StringNode*>(s1);

	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(pOld->m_str));

	pOld->Release();

	pNode->m_str += wsz;
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cmp1,2,((in),const void*,s1,(in),const void*,s2))
{
	return ACE_OS::strcmp(static_cast<const StringNode*>(s1)->m_str.c_str(),static_cast<const StringNode*>(s2)->m_str.c_str());
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cmp2,2,((in),const void*,s1,(in),const char*,sz))
{
	return ACE_OS::strcmp(static_cast<const StringNode*>(s1)->m_str.c_str(),ACE_Ascii_To_Wide(sz).wchar_rep());
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cmp3,2,((in),const void*,s1,(in),const wchar_t*,wsz))
{
	return ACE_OS::strcmp(static_cast<const StringNode*>(s1)->m_str.c_str(),wsz);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cnc1,2,((in),const void*,s1,(in),const void*,s2))
{
	return ACE_OS::strcasecmp(static_cast<const StringNode*>(s1)->m_str.c_str(),static_cast<const StringNode*>(s2)->m_str.c_str());
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cnc2,2,((in),const void*,s1,(in),const char*,sz))
{
	return ACE_OS::strcasecmp(static_cast<const StringNode*>(s1)->m_str.c_str(),ACE_Ascii_To_Wide(sz).wchar_rep());
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cnc3,2,((in),const void*,s1,(in),const wchar_t*,wsz))
{
	return ACE_OS::strcasecmp(static_cast<const StringNode*>(s1)->m_str.c_str(),wsz);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool,string_t_isempty,1,((in),const void*,s1))
{
	return (static_cast<const StringNode*>(s1)->m_str.length() == 0);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_tolower,1,((in),const void*,s1))
{
	wchar_t* pszNew = ACE_OS::strdup(static_cast<const StringNode*>(s1)->m_str.c_str());
	if (!pszNew)
		return 0;

	for (wchar_t* p=pszNew;*p!=L'\0';++p)
	{
		*p = static_cast<wchar_t>(ACE_OS::ace_towlower(*p));
	}

	StringNode* s2;
	try
	{
		OMEGA_NEW(s2,StringNode(pszNew));
	}
	catch (...)
	{
		ACE_OS::free(pszNew);
		throw;
	}

	ACE_OS::free(pszNew);
	return s2;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_toupper,1,((in),const void*,s1))
{
	wchar_t* pszNew = ACE_OS::strdup(static_cast<const StringNode*>(s1)->m_str.c_str());
	if (!pszNew)
		return 0;

	for (wchar_t* p=pszNew;*p!=L'\0';++p)
	{
		*p = static_cast<wchar_t>(ACE_OS::ace_towupper(*p));
	}

	StringNode* s2;
	try
	{
		OMEGA_NEW(s2,StringNode(pszNew));
	}
	catch (...)
	{
		ACE_OS::free(pszNew);
		throw;
	}

	ACE_OS::free(pszNew);
	return s2;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_find1,3,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos))
{
	return static_cast<const StringNode*>(s1)->m_str.find(static_cast<const StringNode*>(s2)->m_str,pos);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_find2,4,((in),const void*,s1,(in),char,c,(in),size_t,pos,(in),bool,bIgnoreCase))
{
	ACE_WString str2 = ACE_Ascii_To_Wide(ACE_CString(c).c_str()).wchar_rep();
	wchar_t c2 = str2[0];

	if (bIgnoreCase)
		c2 = static_cast<wchar_t>(ACE_OS::ace_towlower(c2));

	return static_cast<const StringNode*>(s1)->m_str.find(c2,pos);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_find3,4,((in),const void*,s1,(in),wchar_t,c,(in),size_t,pos,(in),bool,bIgnoreCase))
{
	return static_cast<const StringNode*>(s1)->m_str.find(bIgnoreCase ? static_cast<wchar_t>(ACE_OS::ace_towlower(c)) : c,pos);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_rfind1,4,((in),const void*,s1,(in),char,c,(in),size_t,pos,(in),bool,bIgnoreCase))
{
	ACE_WString str2 = ACE_Ascii_To_Wide(ACE_CString(c).c_str()).wchar_rep();
	wchar_t c2 = str2[0];

	if (bIgnoreCase)
		c2 = static_cast<wchar_t>(ACE_OS::ace_towlower(c2));

	return static_cast<const StringNode*>(s1)->m_str.rfind(c2,pos);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_rfind2,4,((in),const void*,s1,(in),wchar_t,c,(in),size_t,pos,(in),bool,bIgnoreCase))
{
	return static_cast<const StringNode*>(s1)->m_str.rfind(bIgnoreCase ? static_cast<wchar_t>(ACE_OS::ace_towlower(c)) : c,pos);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_len,1,((in),const void*,s1))
{
	return static_cast<const StringNode*>(s1)->m_str.length();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_left,2,((in),const void*,s1,(in),size_t,length))
{
	StringNode* s2;
	OMEGA_NEW(s2,StringNode(static_cast<const StringNode*>(s1)->m_str.substr(0,length)));
	return s2;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_mid,3,((in),const void*,s1,(in),size_t,start,(in),size_t,length))
{
	StringNode* s2;
	OMEGA_NEW(s2,StringNode(static_cast<const StringNode*>(s1)->m_str.substr(start,length)));
	return s2;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_right,2,((in),const void*,s1,(in),size_t,length))
{
	size_t start = static_cast<const StringNode*>(s1)->m_str.length();
	if (length > start)
		start = 0;
	else
		start -= length;

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(static_cast<const StringNode*>(s1)->m_str.substr(start)));
	return s2;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_format,2,((in),const wchar_t*,sz,(in),va_list*,ap))
{
	for (size_t len=256;len<=(size_t)-1 / sizeof(wchar_t);len*=2)
	{
		wchar_t* buf = 0;
		OMEGA_NEW(buf,wchar_t[len]);

		int len2 = ACE_OS::vsnprintf(buf,len,sz,*ap);
		if (len2 >= 0 && static_cast<size_t>(len2) <= len)
		{
			StringNode* s1;
			OMEGA_NEW(s1,StringNode(ACE_WString(buf,len2)));

			delete [] buf;
			return s1;
		}

		delete [] buf;
	}

	return 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(string_t_clear,1,((in),void*,s1))
{
	static_cast<StringNode*>(s1)->m_str.clear();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool,guid_t_eq,2,((in),const Omega::guid_t&,lhs,(in),const Omega::guid_t&,rhs))
{
	return (lhs.Data1==rhs.Data1 && lhs.Data2==rhs.Data2 && lhs.Data3==rhs.Data3 && ACE_OS::memcmp(lhs.Data4,rhs.Data4,8)==0);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool,guid_t_less,2,((in),const Omega::guid_t&,lhs,(in),const Omega::guid_t&,rhs))
{
	return (ACE_OS::memcmp(&lhs,&rhs,sizeof(Omega::guid_t)) < 0);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::guid_t,guid_t_from_string,1,((in),const wchar_t*,sz))
{
	// We use an array here because sscanf reads int's...
	long data0 = 0;
	int data[11] = { 0 };

#if defined (ACE_HAS_TR24731_2005_CRT)
	if (swscanf_s(sz,
#else
	if (swscanf(sz,
#endif
		L"{%8lx-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x}",
		&data0,
		&data[0],
		&data[1],
		&data[2],
		&data[3],
		&data[4],
		&data[5],
		&data[6],
		&data[7],
		&data[8],
		&data[9]) != 11)
	{
		return Omega::guid_t::Null();
	}

	Omega::guid_t guid;
	guid.Data1 = data0;
	guid.Data2 = static_cast<Omega::uint16_t>(data[0]);
	guid.Data3 = static_cast<Omega::uint16_t>(data[1]);
	guid.Data4[0] = static_cast<Omega::byte_t>(data[2]);
	guid.Data4[1] = static_cast<Omega::byte_t>(data[3]);
	guid.Data4[2] = static_cast<Omega::byte_t>(data[4]);
	guid.Data4[3] = static_cast<Omega::byte_t>(data[5]);
	guid.Data4[4] = static_cast<Omega::byte_t>(data[6]);
	guid.Data4[5] = static_cast<Omega::byte_t>(data[7]);
	guid.Data4[6] = static_cast<Omega::byte_t>(data[8]);
	guid.Data4[7] = static_cast<Omega::byte_t>(data[9]);

	return guid;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::guid_t,guid_t_create,0,())
{
#if defined(OMEGA_WIN32)
	UUID uuid = {0,0,0, {0,0,0,0,0,0,0,0} };
	UuidCreate(&uuid);

	Omega::guid_t guid;
	guid = *(Omega::guid_t*)(&uuid);
	return guid;

#else
	static bool bInit = false;
	if (!bInit)
	{
		// We don't care if this gets called twice...
		ACE_Utils::UUID_GENERATOR::instance()->init();
		bInit = true;
	}

	ACE_Utils::UUID uuid;
	ACE_Utils::UUID_GENERATOR::instance()->generateUUID(uuid);

	const ACE_CString* pStr = uuid.to_string();
	if (!pStr)
		OOCORE_THROW_LASTERROR();

	wchar_t szBuf[64];
	ACE_OS::sprintf(szBuf,L"{%hs}",pStr->c_str());

	return Omega::guid_t::FromString(szBuf);
#endif
}
