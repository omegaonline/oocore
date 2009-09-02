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

#include "OOCore_precomp.h"

namespace
{
	struct StringNode
	{
		StringNode(const char* sz) : m_str(OOBase::from_utf8(sz)), m_refcount(1)
		{}

		StringNode(const wchar_t* sz) : m_str(sz), m_refcount(1)
		{}

		StringNode(const wchar_t* sz, size_t length) : m_str(sz,length), m_refcount(1)
		{}

		StringNode(const std::wstring& s) : m_str(s), m_refcount(1)
		{}

		void* AddRef() const
		{
			++const_cast<StringNode*>(this)->m_refcount;
			return const_cast<StringNode*>(this);
		}

		void Release()
		{
			if (--m_refcount==0)
				delete this;
		}

		std::wstring m_str;

	private:
		OOBase::AtomicInt<size_t> m_refcount;
	};
}

// Forward declare the md5 stuff
extern "C"
{
	typedef char MD5Context[88];
	void MD5Init(MD5Context *pCtx);
	void MD5Update(MD5Context *pCtx, const unsigned char *buf, unsigned int len);
	void MD5Final(unsigned char digest[16], MD5Context *pCtx);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor1,2,((in),const char*,sz,(in),int,bUTF8))
{
	if (!sz)
		return 0;

	StringNode* pNode;
	if (bUTF8)
		OMEGA_NEW(pNode,StringNode(sz));
	else
		OMEGA_NEW(pNode,StringNode(OOBase::from_native(sz).c_str()));

	return pNode;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor2,1,((in),const void*,s1))
{
	if (!s1)
		return 0;

	return static_cast<const StringNode*>(s1)->AddRef();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor3,2,((in),const wchar_t*,wsz,(in),size_t,length))
{
	if (!wsz)
		return 0;

	StringNode* pNode;
	if (length == Omega::string_t::npos)
		OMEGA_NEW(pNode,StringNode(wsz));
	else
		OMEGA_NEW(pNode,StringNode(wsz,length));
	return pNode;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t__dctor,1,((in),void*,s1))
{
	if (s1)
		static_cast<StringNode*>(s1)->Release();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_assign,2,((in),void*,s1,(in),const void*,s2))
{
	if (s1)
		static_cast<StringNode*>(s1)->Release();

	if (!s2)
		return 0;

	return static_cast<const StringNode*>(s2)->AddRef();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const wchar_t*,OOCore_string_t_cast,1,((in),const void*,s1))
{
	if (!s1)
		return 0;

	return static_cast<const StringNode*>(s1)->m_str.c_str();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_toutf8,3,((in),const void*,h,(in),char*,sz,(in),size_t,size))
{
	std::string str;

	if (h)
		str = OOBase::to_utf8(static_cast<const StringNode*>(h)->m_str.c_str());

	size_t len = str.length();
	if (len >= size)
		len = size-1;

	memcpy(sz,str.c_str(),len);
	sz[len] = '\0';

	return str.length() + 1;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_add,2,((in),void*,s1,(in),const void*,s2))
{
	if (!s2)
	{
		if (!s1)
			return 0;
		else
		{
			static_cast<StringNode*>(s1)->AddRef();
			return s1;
		}
	}

	StringNode* pNode;
	if (s1)
	{
		StringNode* pOld = static_cast<StringNode*>(s1);
		OMEGA_NEW(pNode,StringNode(pOld->m_str + static_cast<const StringNode*>(s2)->m_str));
		pOld->Release();
	}
	else
		OMEGA_NEW(pNode,StringNode(static_cast<const StringNode*>(s2)->m_str));

	return pNode;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cmp,2,((in),const void*,s1,(in),const void*,s2))
{
	return (s1 ? static_cast<const StringNode*>(s1)->m_str : std::wstring()).compare((s2 ? static_cast<const StringNode*>(s2)->m_str : std::wstring()));
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cnc,2,((in),const void*,s1,(in),const void*,s2))
{
	const wchar_t* str1 = (s1 ? static_cast<const StringNode*>(s1)->m_str.c_str() : 0);
	const wchar_t* str2 = (s2 ? static_cast<const StringNode*>(s2)->m_str.c_str() : 0);

#if defined(HAVE_WCSICMP)
	return wcsicmp(str1,str2);
#elif defined(HAVE_WCSCASECMP)
	return wcscasecmp(str1,str2);
#else
#error Fix me!
#endif
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_isempty,1,((in),const void*,s1))
{
	if (!s1)
		return 1;
	else
		return (static_cast<const StringNode*>(s1)->m_str.empty() ? 1 : 0);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_tolower,1,((in),const void*,s1))
{
	if (!s1)
		return 0;

	OOBase::SmartPtr<wchar_t,OOBase::FreeDestructor<wchar_t> > ptrNew = wcsdup(static_cast<const StringNode*>(s1)->m_str.c_str());
	if (!ptrNew)
		return 0;

#if defined(HAVE_WCSLWR)
	wcslwr(ptrNew.value());
#else
	for (wchar_t* p=ptrNew.value();*p!=L'\0';++p)
		*p = towlower(*p);
#endif

	StringNode* s2 = 0;
	OMEGA_NEW(s2,StringNode(ptrNew.value()));
	return s2;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_toupper,1,((in),const void*,s1))
{
	if (!s1)
		return 0;

	OOBase::SmartPtr<wchar_t,OOBase::FreeDestructor<wchar_t> > ptrNew = wcsdup(static_cast<const StringNode*>(s1)->m_str.c_str());
	if (!ptrNew)
		return 0;

#if defined(HAVE_WCSUPR)
	wcsupr(ptrNew.value());
#else
	for (wchar_t* p=ptrNew.value();*p!=L'\0';++p)
		*p = towupper(*p);
#endif

	StringNode* s2 = 0;
	OMEGA_NEW(s2,StringNode(ptrNew.value()));
	return s2;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find1,3,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos))
{
	if (!s1 || !s2)
		return (size_t)-1;

	return static_cast<const StringNode*>(s1)->m_str.find(static_cast<const StringNode*>(s2)->m_str,pos);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find2,4,((in),const void*,s1,(in),wchar_t,c,(in),size_t,pos,(in),int,bIgnoreCase))
{
	if (!s1)
		return (size_t)-1;

	return static_cast<const StringNode*>(s1)->m_str.find(bIgnoreCase != 0 ? static_cast<wchar_t>(tolower(c)) : c,pos);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_rfind,4,((in),const void*,s1,(in),wchar_t,c,(in),size_t,pos,(in),int,bIgnoreCase))
{
	if (!s1)
		return (size_t)-1;

	return static_cast<const StringNode*>(s1)->m_str.rfind(bIgnoreCase != 0 ? static_cast<wchar_t>(tolower(c)) : c,pos);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_len,1,((in),const void*,s1))
{
	if (!s1)
		return 0;

	return static_cast<const StringNode*>(s1)->m_str.length();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_left,2,((in),const void*,s1,(in),size_t,length))
{
	if (!s1)
		return 0;

	const StringNode* s = static_cast<const StringNode*>(s1);
	if (length == s->m_str.length())
		return s->AddRef();

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(s->m_str.substr(0,length)));
	return s2;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_mid,3,((in),const void*,s1,(in),size_t,start,(in),size_t,length))
{
	if (!s1)
		return 0;

	const StringNode* s = static_cast<const StringNode*>(s1);
	if (start == 0 && length == s->m_str.length())
		return s->AddRef();

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(s->m_str.substr(start,length)));
	return s2;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_right,2,((in),const void*,s1,(in),size_t,length))
{
	if (!s1)
		return 0;

	const StringNode* s = static_cast<const StringNode*>(s1);
	if (length == 0)
		return s->AddRef();

	size_t start = s->m_str.length();
	if (length >= start)
		start = 0;
	else
		start -= length;

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(s->m_str.substr(start)));
	return s2;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_format,2,((in),const wchar_t*,sz,(in),va_list*,ap))
{
	if (!sz)
		return 0;

	for (size_t len=256;len<=(size_t)-1 / sizeof(wchar_t);)
	{
		OOBase::SmartPtr<wchar_t,OOBase::ArrayDestructor<wchar_t> > buf = 0;
		OMEGA_NEW(buf,wchar_t[len]);

		int len2 = vswprintf_s(buf.value(),len,sz,*ap);
		if (len2 > -1 && static_cast<size_t>(len2) < len)
		{
			StringNode* s1;
			OMEGA_NEW(s1,StringNode(std::wstring(buf.value(),len2)));
			return s1;
		}

		if (len2 > -1)
			len = len2 + 1;
		else
			len *= 2;
	}

	return 0;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_clear,1,((in),void*,s1))
{
	if (s1)
		static_cast<StringNode*>(s1)->Release();
}

#if !defined(HAVE_UUID_UUID_H)
namespace
{
	static unsigned int parse(wchar_t c)
	{
		if (c >= L'0' && c <= L'9')
			return (c-L'0');
		else if (c >= L'A' && c <= L'F')
			return (c-L'A'+10);
		else if (c >= L'a' && c <= L'f')
			return (c-L'a'+10);
		else
			throw int(0);
	}
}
#endif

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::string_t,OOCore_guid_t_to_string,1,((in),const Omega::guid_t&,guid))
{
#if defined(HAVE_UUID_UUID_H)

	char szBuf[] = "{00000000-0000-0000-0000-000000000000}";
	uuid_unparse_upper(*(const uuid_t*)(&guid),szBuf+1);
	szBuf[37] = '}';
	return Omega::string_t(szBuf,true);

#else

	return Omega::string_t::Format(L"{%8.8lX-%4.4hX-%4.4hX-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X}",
		guid.Data1,
		guid.Data2,
		guid.Data3,
		guid.Data4[0],
		guid.Data4[1],
		guid.Data4[2],
		guid.Data4[3],
		guid.Data4[4],
		guid.Data4[5],
		guid.Data4[6],
		guid.Data4[7]);

#endif
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,OOCore_guid_t_from_string,2,((in),const wchar_t*,sz,(out),Omega::guid_t&,result))
{
#if defined(HAVE_UUID_UUID_H)

	std::string str = OOBase::to_utf8(sz);
	if (str.length() != 38 || str[0] != '{' || str[37] != '}')
		return 0;

	uuid_t uuid;
	if (uuid_parse(str.substr(1,36).c_str(),uuid))
		return 0;

	result = *(Omega::guid_t*)(uuid);
	return 1;

#else

	// Do this manually...
	result.Data1 = 0;
	result.Data2 = 0;
	result.Data3 = 0;
	memset(result.Data4,sizeof(result.Data4),0);

	if (sz[0] != L'{')
		return 0;

	try
	{
		unsigned int v = (parse(sz[1]) << 28);
		v += (parse(sz[2]) << 24);
		v += (parse(sz[3]) << 20);
		v += (parse(sz[4]) << 16);
		v += (parse(sz[5]) << 12);
		v += (parse(sz[6]) << 8);
		v += (parse(sz[7]) << 4);
		v += parse(sz[8]);
		result.Data1 = static_cast<Omega::uint32_t>(v);

		if (sz[9] != L'-')
			return 0;

		v = (parse(sz[10]) << 12);
		v += (parse(sz[11]) << 8);
		v += (parse(sz[12]) << 4);
		v += parse(sz[13]);
		result.Data2 = static_cast<Omega::uint16_t>(v);

		if (sz[14] != L'-')
			return 0;

		v = (parse(sz[15]) << 12);
		v += (parse(sz[16]) << 8);
		v += (parse(sz[17]) << 4);
		v += parse(sz[18]);
		result.Data3 = static_cast<Omega::uint16_t>(v);

		if (sz[19] != L'-')
			return 0;

		result.Data4[0] = static_cast<Omega::byte_t>((parse(sz[20]) << 4) + parse(sz[21]));
		result.Data4[1] = static_cast<Omega::byte_t>((parse(sz[22]) << 4) + parse(sz[23]));

		if (sz[24] != L'-')
			return false;

		result.Data4[2] = static_cast<Omega::byte_t>((parse(sz[25]) << 4) + parse(sz[26]));
		result.Data4[3] = static_cast<Omega::byte_t>((parse(sz[27]) << 4) + parse(sz[28]));
		result.Data4[4] = static_cast<Omega::byte_t>((parse(sz[29]) << 4) + parse(sz[30]));
		result.Data4[5] = static_cast<Omega::byte_t>((parse(sz[31]) << 4) + parse(sz[32]));
		result.Data4[6] = static_cast<Omega::byte_t>((parse(sz[33]) << 4) + parse(sz[34]));
		result.Data4[7] = static_cast<Omega::byte_t>((parse(sz[35]) << 4) + parse(sz[36]));

		if (sz[37] != L'}' || sz[38] != L'\0')
			return 0;

		return 1;
	}
	catch (int)
	{
		return 0;
	}

#endif
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::guid_t,OOCore_guid_t_create,0,())
{
#if defined(_WIN32)

	UUID uuid = {0,0,0, {0,0,0,0,0,0,0,0} };
	UuidCreate(&uuid);

	return *(Omega::guid_t*)(&uuid);

#elif defined(HAVE_UUID_UUID_H)

	uuid_t uuid = {0};
	uuid_generate(uuid);

	if (uuid_type(uuid) == UUID_TYPE_DCE_RANDOM)
		return *(Omega::guid_t*)(uuid);

	// MD5 hash the result... it hides the MAC address
	MD5Context ctx;
	MD5Init(&ctx);
	MD5Update(&ctx,uuid,sizeof(uuid));

	unsigned char digest[16];
	MD5Final(digest,&ctx);

	return *(Omega::guid_t*)(digest);

#else

#error Fix me!

	// Pull from /dev/random ?

#endif
}
