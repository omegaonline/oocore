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
		StringNode(size_t length) : m_buf(NULL), m_len(0), m_own(true), m_fs(NULL), m_refcount(1)
		{
			assert(length);

			m_buf = static_cast<wchar_t*>(OOBase::HeapAllocate((length+1)*sizeof(wchar_t)));
			m_buf[length] = L'\0';
			m_len = length;
		}

		StringNode(const wchar_t* sz, size_t length, bool own) : m_buf(NULL), m_len(0), m_own(own), m_fs(NULL), m_refcount(1)
		{
			assert(sz);
			assert(length);

			if (m_own)
			{
				m_buf = static_cast<wchar_t*>(OOBase::HeapAllocate((length+1)*sizeof(wchar_t)));
				memcpy(m_buf,sz,length*sizeof(wchar_t));
				m_buf[length] = L'\0';
			}
			else
				m_buf = const_cast<wchar_t*>(sz);

			m_len = length;
		}

		StringNode(const wchar_t* sz1, size_t len1, const wchar_t* sz2, size_t len2) : m_buf(NULL), m_len(0), m_own(true), m_fs(NULL), m_refcount(1)
		{
			assert(sz1);
			assert(len1);
			assert(sz2);
			assert(len2);

			m_buf = static_cast<wchar_t*>(OOBase::HeapAllocate((len1+len2+1)*sizeof(wchar_t)));
			memcpy(m_buf,sz1,len1*sizeof(wchar_t));
			memcpy(m_buf+len1,sz2,len2*sizeof(wchar_t));
			m_buf[len1+len2] = L'\0';
			m_len = len1+len2;
		}

		void* AddRef() const
		{
			++const_cast<StringNode*>(this)->m_refcount;
			return const_cast<StringNode*>(this);
		}

		void* Own() const
		{
			if (m_own)
				return AddRef();

			void* r = new (std::nothrow) StringNode(m_buf,m_len,true);
			if (!r)
				OMEGA_THROW_NOMEM();

			return r;
		}

		void Release()
		{
			if (--m_refcount==0)
				delete this;
		}

		wchar_t* m_buf;
		size_t   m_len;
		bool     m_own;

		struct format_state_t
		{
			struct insert_t
			{
				uint32_t        index;
				int32_t         alignment;
				string_t        format;
				string_t        suffix;
			};
			OOBase::Stack<insert_t>* m_listInserts;
			size_t                   m_curr_arg;
			string_t                 m_prefix;
		};
		format_state_t* m_fs;

		void parse_format();

	private:
		OOBase::Atomic<size_t> m_refcount;

		~StringNode()
		{
			if (m_own)
				OOBase::HeapFree(m_buf);

			if (m_fs)
			{
				delete m_fs->m_listInserts;
				delete m_fs;
			}
		}

		size_t find_brace(size_t start, wchar_t brace);
		void merge_braces(string_t& str);
		void parse_arg(size_t& pos);
	};

	string_t align(const string_t& str, int align)
	{
		unsigned width = (align < 0 ? -align : align);
		if (str.Length() >= width)
			return str;

		string_t strFill;
		for (size_t i=0;i<width-str.Length();++i)
			strFill += L' ';

		if (align < 0)
			return str + strFill;
		else
			return strFill + str;
	}

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
	if (!sz)
		return 0;

	size_t wlen = 0;
	if (bUTF8)
		wlen = OOBase::measure_utf8(sz,len);
	else
		wlen = OOBase::measure_native(sz,len);

	// Remove any trailing null terminator
	if (len == size_t(-1))
		--wlen;

	if (!wlen)
		return 0;

	StringNode* pNode = new (std::nothrow) StringNode(wlen);
	if (!pNode)
		OMEGA_THROW_NOMEM();

	if (bUTF8)
		OOBase::from_utf8(pNode->m_buf,pNode->m_len+1,sz,len);
	else
		OOBase::from_native(pNode->m_buf,pNode->m_len+1,sz,len);
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
		return 0;

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
		return 0;

	return static_cast<const StringNode*>(s2)->AddRef();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const wchar_t*,OOCore_string_t_cast,1,((in),const void*,s1))
{
	assert(s1);

	return static_cast<const StringNode*>(s1)->m_buf;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_toutf8,3,((in),const void*,h,(in),char*,sz,(in),size_t,size))
{
	if (!h)
	{
		if (sz && size)
			*sz = '\0';
		return 1;
	}

	return 1 + OOBase::to_utf8(sz,size,static_cast<const StringNode*>(h)->m_buf,static_cast<const StringNode*>(h)->m_len);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_tonative,3,((in),const void*,h,(in),char*,sz,(in),size_t,size))
{
	if (!h)
	{
		if (sz && size)
			*sz = '\0';
		return 1;
	}

	return 1 + OOBase::to_native(sz,size,static_cast<const StringNode*>(h)->m_buf,static_cast<const StringNode*>(h)->m_len);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_add1,2,((in),void*,s1,(in),const void*,s2))
{
	StringNode* pOrig = static_cast<StringNode*>(s1);
	const StringNode* pAdd = static_cast<const StringNode*>(s2);

	if (!pOrig)
		return pAdd->Own();

	StringNode* pNode = new (std::nothrow) StringNode(pOrig->m_buf,pOrig->m_len,pAdd->m_buf,pAdd->m_len);
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

	StringNode* pNode = new (std::nothrow) StringNode(pOrig->m_buf,pOrig->m_len,&c,1);
	if (!pNode)
		OMEGA_THROW_NOMEM();

	pOrig->Release();

	return pNode;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cmp1,5,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos,(in),size_t,length,(in),int,bIgnoreCase))
{
	const StringNode* s = static_cast<const StringNode*>(s1);

	const wchar_t* p1 = s->m_buf + pos;
	const wchar_t* end1 = s->m_buf + s->m_len;

	const wchar_t* p2 = static_cast<const StringNode*>(s2)->m_buf;
	const wchar_t* end2 = p2 + static_cast<const StringNode*>(s2)->m_len;

	if (length != string_t::npos && length < s->m_len - pos)
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

	const wchar_t* p1 = s->m_buf + pos;
	const wchar_t* end1 = s->m_buf + s->m_len;

	if (length != string_t::npos && length < s->m_len - pos)
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
		return 0;

	StringNode* s2 = new (std::nothrow) StringNode(static_cast<const StringNode*>(s1)->m_buf,static_cast<const StringNode*>(s1)->m_len,true);
	if (!s2)
		OMEGA_THROW_NOMEM();

	for (wchar_t* p=s2->m_buf; size_t(p-s2->m_buf) < s2->m_len; ++p)
		*p = towlower(*p);

	return s2;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_toupper,1,((in),const void*,s1))
{
	if (!s1)
		return 0;

	StringNode* s2 = new (std::nothrow) StringNode(static_cast<const StringNode*>(s1)->m_buf,static_cast<const StringNode*>(s1)->m_len,true);
	if (!s2)
		OMEGA_THROW_NOMEM();

	for (wchar_t* p=s2->m_buf; size_t(p-s2->m_buf) < s2->m_len; ++p)
		*p = towupper(*p);

	return s2;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find1,4,((in),const void*,s1,(in),wchar_t,c,(in),size_t,pos,(in),int,bIgnoreCase))
{
	size_t len = static_cast<const StringNode*>(s1)->m_len;
	if (pos >= len)
		return string_t::npos;

	const wchar_t* st = static_cast<const StringNode*>(s1)->m_buf;
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
	size_t len = static_cast<const StringNode*>(s1)->m_len;
	if (pos >= len)
		return string_t::npos;

	const wchar_t* st = static_cast<const StringNode*>(s1)->m_buf;
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
	const wchar_t* st = static_cast<const StringNode*>(s2)->m_buf;
	size_t len = static_cast<const StringNode*>(s2)->m_len;

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
	size_t len = static_cast<const StringNode*>(s1)->m_len;
	if (pos >= len)
		return string_t::npos;

	const wchar_t* st = static_cast<const StringNode*>(s1)->m_buf;
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
	size_t len = static_cast<const StringNode*>(s1)->m_len;
	if (pos >= len)
		return string_t::npos;

	const wchar_t* st = static_cast<const StringNode*>(s1)->m_buf;
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
	size_t len = static_cast<const StringNode*>(s1)->m_len;
	if (pos >= len)
		pos = len - 1;

	const wchar_t* st = static_cast<const StringNode*>(s1)->m_buf;
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
	return static_cast<const StringNode*>(s1)->m_len;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_left,2,((in),const void*,s1,(in),size_t,length))
{
	const StringNode* s = static_cast<const StringNode*>(s1);
	if (length >= s->m_len)
		return s->Own();

	void* r = new (std::nothrow) StringNode(s->m_buf,length,true);
	if (!r)
		OMEGA_THROW_NOMEM();

	return r;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_mid,3,((in),const void*,s1,(in),size_t,start,(in),size_t,length))
{
	const StringNode* s = static_cast<const StringNode*>(s1);
	if (start > s->m_len)
		return 0;

	if (length >= s->m_len - start)
		length = s->m_len - start;

	if (length == 0)
		return 0;

	if (start == 0 && length == s->m_len)
		return s->Own();

	void* r = new (std::nothrow) StringNode(s->m_buf + start,length,true);
	if (!r)
		OMEGA_THROW_NOMEM();

	return r;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_right,2,((in),const void*,s1,(in),size_t,length))
{
	const StringNode* s = static_cast<const StringNode*>(s1);
	if (length >= s->m_len)
		return s->Own();

	void* r = new (std::nothrow) StringNode(s->m_buf + s->m_len-length,length,true);
	if (!r)
		OMEGA_THROW_NOMEM();

	return r;
}

void StringNode::parse_arg(size_t& pos)
{
	size_t end = find_brace(pos,L'}');
	if (end == string_t::npos)
		throw Formatting::IFormattingException::Create(L"Missing matching '}' in format string: {0}" % string_t(m_buf,m_len));

	size_t comma = OOCore_string_t_find1_Impl(this,L',',pos,0);
	size_t colon = OOCore_string_t_find1_Impl(this,L':',pos,0);
	if (comma == pos || colon == pos)
		throw Formatting::IFormattingException::Create(L"Missing index in format string: {0}" % string_t(m_buf,m_len));

	format_state_t::insert_t ins;
	ins.alignment = 0;

	const wchar_t* endp = 0;
	ins.index = OOCore::wcstou32(m_buf+pos,endp,10);
	if (ins.index < m_fs->m_curr_arg)
		m_fs->m_curr_arg = ins.index;

	if (comma < end && comma < colon)
	{
		pos = comma++;
		ins.alignment = OOCore::wcsto32(m_buf+comma,endp,10);
	}

	if (colon < end)
	{
		ins.format = string_t(m_buf+colon+1,end-colon-1);
		merge_braces(ins.format);
	}

	m_fs->m_listInserts->push(ins);

	pos = end + 1;
}

void StringNode::merge_braces(string_t& str)
{
	for (size_t pos = 0;;)
	{
		pos = str.Find(L'{',pos);
		if (pos == string_t::npos)
			break;

		str = str.Left(pos) + str.Mid(pos+1);
	}

	for (size_t pos = 0;;)
	{
		pos = str.Find(L'}',pos);
		if (pos == string_t::npos)
			break;

		str = str.Left(pos) + str.Mid(pos+1);
	}
}

size_t StringNode::find_brace(size_t start, wchar_t brace)
{
	for (;;)
	{
		size_t found = OOCore_string_t_find1_Impl(this,brace,start,0);
		if (found == string_t::npos)
			return string_t::npos;

		if (found < m_len && m_buf[found+1] != brace)
			return found;

		// Skip {{
		start = found + 2;
	}
}

void StringNode::parse_format()
{
	// Prefix first
	size_t pos = find_brace(0,L'{');
	if (pos == string_t::npos)
		throw Formatting::IFormattingException::Create(L"No inserts in format string: {0}" % string_t(m_buf,m_len));

	m_fs = new (std::nothrow) format_state_t;
	if (!m_fs)
		OMEGA_THROW_NOMEM();

	m_fs->m_listInserts = new (std::nothrow) OOBase::Stack<format_state_t::insert_t>();
	if (!m_fs->m_listInserts)
		OMEGA_THROW_NOMEM();

	m_fs->m_curr_arg = (size_t)-1;
	m_fs->m_prefix = string_t(m_buf,pos++);
	merge_braces(m_fs->m_prefix);

	// Parse args
	for (;;)
	{
		parse_arg(pos);

		size_t found = find_brace(pos,L'{');

		string_t suffix;
		if (found == string_t::npos)
			suffix = string_t(m_buf+pos,size_t(-1));
		else
			suffix = string_t(m_buf+pos,found-pos);

		merge_braces(suffix);

		m_fs->m_listInserts->at(m_fs->m_listInserts->size()-1)->suffix = suffix;

		if (found == string_t::npos)
			break;

		pos = found + 1;
	}

	for (size_t idx = m_fs->m_curr_arg;;)
	{
		size_t count = 0;
		bool bFound = false;
		for (size_t i = 0;i<m_fs->m_listInserts->size(); ++i)
		{
			format_state_t::insert_t* ins = m_fs->m_listInserts->at(i);

			if (ins->index <= idx)
				++count;

			if (ins->index == idx)
			{
				bFound = true;
				++idx;
				break;
			}
		}

		if (!bFound)
		{
			if (count < m_fs->m_listInserts->size())
				throw Formatting::IFormattingException::Create(L"Index gap in format string: {0}" % string_t(m_buf,m_len));
			break;
		}
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,OOCore_string_t_get_arg,3,((in),size_t,idx,(in_out),void**,s1,(out),Omega::string_t&,fmt))
{
	StringNode* s = static_cast<StringNode*>(*s1);
	if (!s)
		throw Formatting::IFormattingException::Create(L"Empty format string");

	size_t arg;
	if (!idx)
	{
		// Clone s
		StringNode* pNewNode = new (std::nothrow) StringNode(s->m_buf,s->m_len,true);
		if (!pNewNode)
			OMEGA_THROW_NOMEM();

		if (s->m_fs)
		{
			pNewNode->m_fs = new (std::nothrow) StringNode::format_state_t(*s->m_fs);
			if (!pNewNode->m_fs)
				OMEGA_THROW_NOMEM();

			if (s->m_fs->m_listInserts)
			{
				pNewNode->m_fs->m_listInserts = new (std::nothrow) OOBase::Stack<StringNode::format_state_t::insert_t>();
				if (!pNewNode->m_fs->m_listInserts)
					OMEGA_THROW_NOMEM();
				
				for (size_t i=0;i<s->m_fs->m_listInserts->size();++i)
					pNewNode->m_fs->m_listInserts->push(*s->m_fs->m_listInserts->at(i));
			}
		}
		
		s->Release();
		s = pNewNode;
		*s1 = s;

		if (!s->m_fs)
			s->parse_format();

		arg = s->m_fs->m_curr_arg++;
	}
	else
	{
		assert(s->m_fs);
		assert(s->m_fs->m_listInserts);

		arg = s->m_fs->m_curr_arg-1;
	}

	assert(s->m_own);

	for (size_t i=0; i!=s->m_fs->m_listInserts->size(); ++i)
	{
		StringNode::format_state_t::insert_t* ins = s->m_fs->m_listInserts->at(i);

		if (ins->index == arg)
		{
			fmt = ins->format;
			return 1;
		}
	}

	// Now measure how much space we need
	string_t str = s->m_fs->m_prefix;
	for (size_t i=0; i!=s->m_fs->m_listInserts->size(); ++i)
	{
		StringNode::format_state_t::insert_t* ins = s->m_fs->m_listInserts->at(i);

		str += ins->format;
		str += ins->suffix;
	}

	size_t len = str.Length();
	if (len > s->m_len)
	{
		wchar_t* buf_new = static_cast<wchar_t*>(OOBase::HeapAllocate((len+1)*sizeof(wchar_t)));
		buf_new[len] = L'\0';

		OOBase::HeapFree(s->m_buf);
		s->m_buf = buf_new;
	}

	memcpy(s->m_buf,str.c_str(),len*sizeof(wchar_t));
	s->m_buf[len] = L'\0';
	s->m_len = len;
	
	return 0;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_string_t_set_arg,2,((in),void*,s1,(in),const Omega::string_t&,arg))
{
	StringNode* s = static_cast<StringNode*>(s1);

	for (size_t i=0;i<s->m_fs->m_listInserts->size();++i)
	{
		StringNode::format_state_t::insert_t* ins = s->m_fs->m_listInserts->at(i);

		if (ins->index == s->m_fs->m_curr_arg-1)
		{
			ins->index = unsigned int(-1);
			ins->format = arg;
			break;
		}
	}

	while (!s->m_fs->m_listInserts->empty())
	{
		size_t end = s->m_fs->m_listInserts->size()-1;

		StringNode::format_state_t::insert_t* ins = s->m_fs->m_listInserts->at(end);

		if (ins->index != unsigned int(-1))
			break;

		string_t txt = align(ins->format,ins->alignment) + ins->suffix;

		if (end > 0)
		{
			StringNode::format_state_t::insert_t* prev = s->m_fs->m_listInserts->at(end-1);

			prev->suffix += txt;
		}
		else
			s->m_fs->m_prefix += txt;

		s->m_fs->m_listInserts->pop();
	}
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
	const wchar_t* sz = str.c_str();

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
