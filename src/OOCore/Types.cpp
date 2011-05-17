///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007,2011 Rick Taylor
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

#include <OOBase/STLAllocator.h>

#if defined(HAVE_UUID_UUID_H)
#include <uuid/uuid.h>
#endif

using namespace Omega;

namespace
{
	struct StringNode
	{
		enum Flags
		{
			eOwn = 1,
			eUTF8 = 2,
			eNative = 4
		};

		StringNode(size_t length) : m_wbuf(NULL), m_wlen(0), m_cbuf(NULL), m_clen(0), m_flags(eOwn), m_refcount(1)
		{
			assert(length);

			wchar_t* buf = static_cast<wchar_t*>(OOBase::HeapAllocate((length+1)*sizeof(wchar_t)));
			buf[length] = L'\0';
			
			m_wbuf = buf;			
			m_wlen = length;
		}

		StringNode(const wchar_t* sz, size_t length, bool own) : m_wbuf(NULL), m_wlen(0), m_cbuf(NULL), m_clen(0), m_flags(own ? eOwn : 0), m_refcount(1)
		{
			assert(sz);
			assert(length);

			if (own)
			{
				wchar_t* buf = static_cast<wchar_t*>(OOBase::HeapAllocate((length+1)*sizeof(wchar_t)));
				memcpy(buf,sz,length*sizeof(wchar_t));
				buf[length] = L'\0';

				m_wbuf = buf;
			}
			else
				m_wbuf = sz;

			m_wlen = length;
		}

		StringNode(const wchar_t* sz1, size_t len1, const wchar_t* sz2, size_t len2) : m_wbuf(NULL), m_wlen(0), m_cbuf(NULL), m_clen(0), m_flags(eOwn), m_refcount(1)
		{
			assert(sz1);
			assert(len1);
			assert(sz2);
			assert(len2);

			wchar_t* buf = static_cast<wchar_t*>(OOBase::HeapAllocate((len1+len2+1)*sizeof(wchar_t)));
			memcpy(buf,sz1,len1*sizeof(wchar_t));
			memcpy(buf+len1,sz2,len2*sizeof(wchar_t));
			buf[len1+len2] = L'\0';
			
			m_wbuf = buf;
			m_wlen = len1+len2;
		}

		void* AddRef()
		{
			++m_refcount;
			return this;
		}

		void* Own() const
		{
			if (m_flags & eOwn)
				return const_cast<StringNode*>(this)->AddRef();

			void* r = new (std::nothrow) StringNode(m_wbuf,m_wlen,true);
			if (!r)
				OMEGA_THROW_NOMEM();

			return r;
		}

		void Release()
		{
			if (--m_refcount == 0)
				delete this;
		}

		const wchar_t* m_wbuf;
		size_t         m_wlen;
		const char*    m_cbuf;
		size_t         m_clen;
		unsigned int   m_flags;

	private:
		OOBase::Atomic<size_t> m_refcount;

		~StringNode()
		{
			if (m_flags & eOwn)
				OOBase::HeapFree(const_cast<wchar_t*>(m_wbuf));

			if (m_flags & (eUTF8 | eNative))
				OOBase::HeapFree(const_cast<char*>(m_cbuf));
		}
	};

	template <typename T>
	bool any_compare(const any_t& lhs, const any_t& rhs)
	{
		T v1,v2;
		if (lhs.Coerce(v1) != any_t::castValid || rhs.Coerce(v2) != any_t::castValid)
			return false;

		return (v1 == v2);
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor1,3,((in),const char*,sz,(in),size_t,len,(in),int,bUTF8))
{
	if (!sz || !len)
		return NULL;

	wchar_t wszBuf[128] = {0};
	size_t wlen = 0;
	if (bUTF8)
		wlen = OOBase::from_utf8(wszBuf,sizeof(wszBuf)/sizeof(wchar_t),sz,len);
	else
		wlen = OOBase::from_native(wszBuf,sizeof(wszBuf)/sizeof(wchar_t),sz,len);

	// Remove any trailing null terminator
	if (len == size_t(-1) && wlen > 0)
		--wlen;

	StringNode* pNode = NULL;
	if (wlen == 0)
		return pNode;
	
	if (wlen <= sizeof(wszBuf)/sizeof(wchar_t))
	{
		pNode = new (std::nothrow) StringNode(wszBuf,wlen,true);
		if (!pNode)
			OMEGA_THROW_NOMEM();
	}
	else
	{
		pNode = new (std::nothrow) StringNode(wlen);
		if (!pNode)
			OMEGA_THROW_NOMEM();

		if (bUTF8)
			OOBase::from_utf8(const_cast<wchar_t*>(pNode->m_wbuf),pNode->m_wlen+1,sz,len);
		else
			OOBase::from_native(const_cast<wchar_t*>(pNode->m_wbuf),pNode->m_wlen+1,sz,len);
	}

	return pNode;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_addref,2,((in),void*,s1,(in),int,own))
{
	assert(s1);

	if (own == 0)
		return static_cast<StringNode*>(s1)->AddRef();
	else
		return static_cast<StringNode*>(s1)->Own();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor2,3,((in),const wchar_t*,wsz,(in),size_t,length,(in),int,copy))
{
	if (length == string_t::npos)
		length = wcslen(wsz);

	if (!length)
		return NULL;

	void* r = new (std::nothrow) StringNode(wsz,length,copy==0 ? false : true);
	if (!r)
		OMEGA_THROW_NOMEM();

	return r;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_release,1,((in),void*,s1))
{
	assert(s1);

	static_cast<StringNode*>(s1)->Release();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_assign1,2,((in),void*,s1,(in),const void*,s2))
{
	if (s1)
		static_cast<StringNode*>(s1)->Release();

	if (!s2)
		return NULL;

	return static_cast<const StringNode*>(s2)->Own();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const wchar_t*,OOCore_string_t_cast_w,1,((in),const void*,s1))
{
	if (!s1)
		return L"";

	return static_cast<const StringNode*>(s1)->m_wbuf;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const char*,OOCore_string_t_toutf8,2,((in),const void*,h,(out),size_t*,len))
{
	if (!h)
	{
		if (len)
			*len = 1;
		return "";
	}

	StringNode* s = const_cast<StringNode*>(static_cast<const StringNode*>(h));
	if (!(s->m_flags & StringNode::eUTF8))
	{
		char szBuf[256] = {0};
		size_t clen = OOBase::to_utf8(szBuf,sizeof(szBuf),s->m_wbuf,s->m_wlen);
		
		char* cbuf = static_cast<char*>(OOBase::HeapAllocate(clen+1));
		if (!cbuf)
			OMEGA_THROW_NOMEM();

		if (clen < sizeof(szBuf))
			memcpy(cbuf,szBuf,clen+1);
		else
			OOBase::to_utf8(cbuf,clen+1,s->m_wbuf,s->m_wlen);

		cbuf[clen] = '\0';

		if (s->m_flags & StringNode::eNative)
		{
			OOBase::HeapFree(const_cast<char*>(s->m_cbuf));
			s->m_flags &= ~StringNode::eNative;
		}

		s->m_cbuf = cbuf;
		s->m_clen = clen;
		s->m_flags |= StringNode::eUTF8;
	}
		
	if (len)
		*len = s->m_clen;

	return s->m_cbuf;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const char*,OOCore_string_t_tonative,2,((in),const void*,h,(out),size_t*,len))
{
	if (!h)
	{
		if (len)
			*len = 1;
		return "";
	}

	StringNode* s = const_cast<StringNode*>(static_cast<const StringNode*>(h));
	if (!(s->m_flags & StringNode::eNative))
	{
		char szBuf[256] = {0};
		size_t clen = OOBase::to_native(szBuf,sizeof(szBuf),s->m_wbuf,s->m_wlen);
		
		char* cbuf = static_cast<char*>(OOBase::HeapAllocate(clen+1));
		if (!cbuf)
			OMEGA_THROW_NOMEM();

		if (clen < sizeof(szBuf))
			memcpy(cbuf,szBuf,clen+1);
		else
			OOBase::to_native(cbuf,clen+1,s->m_wbuf,s->m_wlen);

		cbuf[clen] = '\0';

		if (s->m_flags & StringNode::eUTF8)
		{
			OOBase::HeapFree(const_cast<char*>(s->m_cbuf));
			s->m_flags &= ~StringNode::eUTF8;
		}

		s->m_cbuf = cbuf;
		s->m_clen = clen;
		s->m_flags |= StringNode::eNative;
	}
		
	if (len)
		*len = s->m_clen;

	return s->m_cbuf;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_add1,2,((in),void*,s1,(in),const void*,s2))
{
	StringNode* pOrig = static_cast<StringNode*>(s1);
	const StringNode* pAdd = static_cast<const StringNode*>(s2);

	if (!pOrig)
		return pAdd->Own();

	StringNode* pNode = new (std::nothrow) StringNode(pOrig->m_wbuf,pOrig->m_wlen,pAdd->m_wbuf,pAdd->m_wlen);
	if (!pNode)
		OMEGA_THROW_NOMEM();

	pOrig->Release();

	return pNode;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_add2,2,((in),void*,s1,(in),wchar_t,c))
{
	StringNode* pOrig = static_cast<StringNode*>(s1);

	if (!pOrig)
		return OOCore_string_t__ctor2(&c,1,1);

	StringNode* pNode = new (std::nothrow) StringNode(pOrig->m_wbuf,pOrig->m_wlen,&c,1);
	if (!pNode)
		OMEGA_THROW_NOMEM();

	pOrig->Release();

	return pNode;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cmp1,5,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos,(in),size_t,length,(in),int,bIgnoreCase))
{
	const StringNode* s = static_cast<const StringNode*>(s1);

	const wchar_t* p1 = s->m_wbuf + pos;
	const wchar_t* end1 = s->m_wbuf + s->m_wlen;

	const wchar_t* p2 = static_cast<const StringNode*>(s2)->m_wbuf;
	const wchar_t* end2 = p2 + static_cast<const StringNode*>(s2)->m_wlen;

	if (length != string_t::npos && length < s->m_wlen - pos)
	{
		end1 = p1 + length;
		end2 = p2 + length;
	}

	wint_t l1 = 0, l2 = 0;
	if (bIgnoreCase)
	{
		while (p1<end1 && p2<end2 && (l1 = towlower(*p1)) == (l2 = towlower(*p2)))
		{
			++p1;
			++p2;
		}
	}
	else
	{
		while (p1<end1 && p2<end2 && (l1 = *p1) == (l2 = *p2))
		{
			++p1;
			++p2;
		}
	}

	if (l1 != l2)
		return (l1 < l2 ? -1 : 1);

	if (p1 < end1)
		return 1;

	if (p2 < end2)
		return -1;

	return 0;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cmp2,5,((in),const void*,s1,(in),const wchar_t*,wsz,(in),size_t,pos,(in),size_t,length,(in),int,bIgnoreCase))
{
	const StringNode* s = static_cast<const StringNode*>(s1);

	const wchar_t* p1 = s->m_wbuf + pos;
	const wchar_t* end1 = s->m_wbuf + s->m_wlen;

	if (length != string_t::npos && length < s->m_wlen - pos)
		end1 = p1 + length;

	wint_t l1 = 0, l2 = 0;
	if (bIgnoreCase)
	{
		while (p1<end1 && *wsz != L'\0' && (l1 = towlower(*p1)) == (l2 = towlower(*wsz)))
		{
			++p1;
			++wsz;
		}
	}
	else
	{
		while (p1<end1 && *wsz != L'\0' && (l1 = *p1) == (l2 = *wsz))
		{
			++p1;
			++wsz;
		}
	}

	if (l1 != l2)
		return (l1 < l2 ? -1 : 1);

	if (p1 < end1)
		return 1;

	if (*wsz != L'\0')
		return -1;

	return 0;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_tolower,1,((in),const void*,s1))
{
	if (!s1)
		return NULL;

	StringNode* s2 = new (std::nothrow) StringNode(static_cast<const StringNode*>(s1)->m_wbuf,static_cast<const StringNode*>(s1)->m_wlen,true);
	if (!s2)
		OMEGA_THROW_NOMEM();

	for (wchar_t* p=const_cast<wchar_t*>(s2->m_wbuf); size_t(p-s2->m_wbuf) < s2->m_wlen; ++p)
		*p = towlower(*p);

	return s2;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_toupper,1,((in),const void*,s1))
{
	if (!s1)
		return NULL;

	StringNode* s2 = new (std::nothrow) StringNode(static_cast<const StringNode*>(s1)->m_wbuf,static_cast<const StringNode*>(s1)->m_wlen,true);
	if (!s2)
		OMEGA_THROW_NOMEM();

	for (wchar_t* p=const_cast<wchar_t*>(s2->m_wbuf); size_t(p-s2->m_wbuf) < s2->m_wlen; ++p)
		*p = towupper(*p);

	return s2;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find1,4,((in),const void*,s1,(in),wchar_t,c,(in),size_t,pos,(in),int,bIgnoreCase))
{
	size_t len = static_cast<const StringNode*>(s1)->m_wlen;
	if (pos >= len)
		return string_t::npos;

	const wchar_t* st = static_cast<const StringNode*>(s1)->m_wbuf;
	const wchar_t* p = st + pos;

	if (bIgnoreCase)
	{
		wint_t ci = towlower(c);
		for (; towlower(*p) != ci && size_t(p-st)<len; ++p)
			;
	}
	else
	{
		for (; *p != c && size_t(p-st)<len; ++p)
			;
	}

	if (size_t(p-st) == len)
		return string_t::npos;
	else
		return size_t(p-st);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_not,4,((in),const void*,s1,(in),wchar_t,c,(in),size_t,pos,(in),int,bIgnoreCase))
{
	size_t len = static_cast<const StringNode*>(s1)->m_wlen;
	if (pos >= len)
		return string_t::npos;

	const wchar_t* st = static_cast<const StringNode*>(s1)->m_wbuf;
	const wchar_t* p = st + pos;

	if (bIgnoreCase)
	{
		wint_t ci = towlower(c);
		for (; towlower(*p) == ci && size_t(p-st)<len; ++p)
			;
	}
	else
	{
		for (; *p == c && size_t(p-st)<len; ++p)
			;
	}

	if (size_t(p-st) == len)
		return string_t::npos;
	else
		return size_t(p-st);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find2,4,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos,(in),int,bIgnoreCase))
{
	const wchar_t* st = static_cast<const StringNode*>(s2)->m_wbuf;
	size_t len = static_cast<const StringNode*>(s2)->m_wlen;

	for (;;)
	{
		size_t start = OOCore_string_t_find1_Impl(s1,*st,pos,bIgnoreCase);
		if (start == string_t::npos)
			break;

		if (OOCore_string_t_cmp1_Impl(s1,s2,start,len,bIgnoreCase) == 0)
			return start;

		pos = start + 1;
	}

	return string_t::npos;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_oneof,4,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos,(in),int,bIgnoreCase))
{
	size_t len = static_cast<const StringNode*>(s1)->m_wlen;
	if (pos >= len)
		return string_t::npos;

	const wchar_t* st = static_cast<const StringNode*>(s1)->m_wbuf;
	const wchar_t* p = st + pos;

	for (; OOCore_string_t_find1_Impl(s2,*p,0,bIgnoreCase) == string_t::npos && size_t(p-st)<len; ++p)
		;

	if (size_t(p-st) == len)
		return string_t::npos;
	else
		return size_t(p-st);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_notof,4,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos,(in),int,bIgnoreCase))
{
	size_t len = static_cast<const StringNode*>(s1)->m_wlen;
	if (pos >= len)
		return string_t::npos;

	const wchar_t* st = static_cast<const StringNode*>(s1)->m_wbuf;
	const wchar_t* p = st + pos;

	for (; OOCore_string_t_find1_Impl(s2,*p,0,bIgnoreCase) != string_t::npos && size_t(p-st)<len; ++p)
		;

	if (size_t(p-st) == len)
		return string_t::npos;
	else
		return size_t(p-st);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_rfind,4,((in),const void*,s1,(in),wchar_t,c,(in),size_t,pos,(in),int,bIgnoreCase))
{
	size_t len = static_cast<const StringNode*>(s1)->m_wlen;
	if (pos >= len)
		pos = len - 1;

	const wchar_t* st = static_cast<const StringNode*>(s1)->m_wbuf;
	const wchar_t* p = st + pos;

	if (bIgnoreCase)
	{
		wint_t ci = towlower(c);
		for (; towlower(*p) != ci && p>=st; --p)
			;
	}
	else
	{
		for (; *p != c && p>=st; --p)
			;
	}

	if (p < st)
		return string_t::npos;
	else
		return size_t(p-st);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_len,1,((in),const void*,s1))
{
	return static_cast<const StringNode*>(s1)->m_wlen;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_left,2,((in),const void*,s1,(in),size_t,length))
{
	const StringNode* s = static_cast<const StringNode*>(s1);
	if (length >= s->m_wlen)
		return s->Own();

	void* r = new (std::nothrow) StringNode(s->m_wbuf,length,true);
	if (!r)
		OMEGA_THROW_NOMEM();

	return r;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_mid,3,((in),const void*,s1,(in),size_t,start,(in),size_t,length))
{
	const StringNode* s = static_cast<const StringNode*>(s1);
	if (start > s->m_wlen)
		return NULL;

	if (length >= s->m_wlen - start)
		length = s->m_wlen - start;

	if (length == 0)
		return NULL;

	if (start == 0 && length == s->m_wlen)
		return s->Own();

	void* r = new (std::nothrow) StringNode(s->m_wbuf + start,length,true);
	if (!r)
		OMEGA_THROW_NOMEM();

	return r;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_right,2,((in),const void*,s1,(in),size_t,length))
{
	const StringNode* s = static_cast<const StringNode*>(s1);
	if (length >= s->m_wlen)
		return s->Own();

	void* r = new (std::nothrow) StringNode(s->m_wbuf + s->m_wlen-length,length,true);
	if (!r)
		OMEGA_THROW_NOMEM();

	return r;
}

#if !defined(_WIN32)
	// Forward declare the md5 stuff
	extern "C"
	{
		typedef char MD5Context[88];
		void MD5Init(MD5Context *pCtx);
		void MD5Update(MD5Context *pCtx, const unsigned char *buf, unsigned int len);
		void MD5Final(unsigned char digest[16], MD5Context *pCtx);
	}
#endif

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,OOCore_guid_t_to_string,2,((in),const guid_t&,guid,(in),const string_t&,strFormat))
{
	OMEGA_UNUSED_ARG(strFormat);

	OOBase::LocalString str;
	int err = str.printf("{%.8X-%.4X-%.4X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X}",
		guid.Data1,guid.Data2,guid.Data3,
		guid.Data4[0],guid.Data4[1],guid.Data4[2],guid.Data4[3],
		guid.Data4[4],guid.Data4[5],guid.Data4[6],guid.Data4[7]);

	if (err != 0)
		OMEGA_THROW(err);

	return string_t(str.c_str(),false);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,OOCore_guid_t_from_string,2,((in),const string_t&,str,(out),guid_t&,result))
{
	const wchar_t* sz = str.c_wstr();

	// Do this manually...
	result.Data1 = 0;
	result.Data2 = 0;
	result.Data3 = 0;
	memset(result.Data4,sizeof(result.Data4),0);

	if (sz[0] != L'{' || !iswxdigit(sz[1]))
		return 0;

	const wchar_t* endp = 0;
	result.Data1 = OOCore::wcstou32(sz+1,endp,16);
	if (endp != sz+9)
		return 0;

	if (sz[9] != L'-' || !iswxdigit(sz[10]))
		return 0;

	result.Data2 = static_cast<uint16_t>(OOCore::wcstou32(sz+10,endp,16));
	if (endp != sz+14 || sz[14] != L'-' || !iswxdigit(sz[15]))
		return 0;

	result.Data3 = static_cast<uint16_t>(OOCore::wcstou32(sz+15,endp,16));
	if (endp != sz+19 || sz[19] != L'-' || !iswxdigit(sz[20]))
		return 0;

	uint32_t v1 = OOCore::wcstou32(sz+20,endp,16);
	if (endp != sz+24)
		return 0;

	result.Data4[0] = static_cast<byte_t>((v1 >> 8) & 0xFF);
	result.Data4[1] = static_cast<byte_t>(v1 & 0xFF);

	if (sz[24] != L'-' || !iswxdigit(sz[25]))
		return 0;

	uint64_t v2 = OOCore::wcstou64(sz+25,endp,16);
	if (endp != sz+37)
		return 0;

	result.Data4[2] = static_cast<byte_t>(((v2 >> 32) >> 8) & 0xFF);
	result.Data4[3] = static_cast<byte_t>((v2 >> 32) & 0xFF);
	result.Data4[4] = static_cast<byte_t>((v2 >> 24) & 0xFF);
	result.Data4[5] = static_cast<byte_t>((v2 >> 16) & 0xFF);
	result.Data4[6] = static_cast<byte_t>((v2 >> 8) & 0xFF);
	result.Data4[7] = static_cast<byte_t>(v2 & 0xFF);

	if (sz[37] != L'}' || sz[38] != L'\0')
		return 0;

	return 1;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(guid_t,OOCore_guid_t_create,0,())
{
#if defined(_WIN32)

	UUID uuid = {0,0,0, {0,0,0,0,0,0,0,0} };
	UuidCreate(&uuid);

	return *(guid_t*)(&uuid);

#elif defined(HAVE_UUID_UUID_H)

	uuid_t uuid = {0};
	uuid_generate(uuid);

	if (uuid_type(uuid) == UUID_TYPE_DCE_RANDOM)
		return *(guid_t*)(uuid);

	// MD5 hash the result... it hides the MAC address
	MD5Context ctx;
	MD5Init(&ctx);
	MD5Update(&ctx,uuid,sizeof(uuid));

	unsigned char digest[16];
	MD5Final(digest,&ctx);

	return *(guid_t*)(digest);

#else

#error Fix me!

	// Pull from /dev/random ?

#endif
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool_t,OOCore_any_t_equal,2,((in),const any_t&,lhs,(in),const any_t&,rhs))
{
	// void comparison
	if (lhs.GetType() == TypeInfo::typeVoid || rhs.GetType() == TypeInfo::typeVoid)
		return (lhs.GetType() == rhs.GetType());

	// guid_t comparison
	if (lhs.GetType() == TypeInfo::typeGuid || rhs.GetType() == TypeInfo::typeGuid)
		return any_compare<guid_t>(lhs,rhs);

	// string_t comparison
	if (lhs.GetType() == TypeInfo::typeString || rhs.GetType() == TypeInfo::typeString)
		return any_compare<string_t>(lhs,rhs);

	// bool_t comparison
	if (lhs.GetType() == TypeInfo::typeBool || rhs.GetType() == TypeInfo::typeBool)
		return any_compare<bool_t>(lhs,rhs);

	// floatX_t comparison
	if (lhs.GetType() == TypeInfo::typeFloat8 || rhs.GetType() == TypeInfo::typeFloat8 ||
			lhs.GetType() == TypeInfo::typeFloat4 || rhs.GetType() == TypeInfo::typeFloat4)
	{
		return any_compare<float8_t>(lhs,rhs);
	}

	// uint64_t comparison - everything else fits in int64_t
	if (lhs.GetType() == TypeInfo::typeUInt64 || rhs.GetType() == TypeInfo::typeUInt64)
		return any_compare<uint64_t>(lhs,rhs);

	// Try comparing as int64_t
	return any_compare<int64_t>(lhs,rhs);
}
