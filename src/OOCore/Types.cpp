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

using namespace Omega;

namespace
{
	struct StringNode
	{
		StringNode(size_t length) : m_buf(0), m_len(0), m_own(true), m_fs(0), m_refcount(1)
		{
			assert(length);

			OMEGA_NEW(m_buf,wchar_t[length+1]);
			m_buf[length] = L'\0';
			m_len = length;
		}

		StringNode(const wchar_t* sz, size_t length, bool own) : m_buf(0), m_len(0), m_own(own), m_fs(0), m_refcount(1)
		{
			assert(sz);
			assert(length);

			if (m_own)
			{
				OMEGA_NEW(m_buf,wchar_t[length+1]);
				memcpy(m_buf,sz,length*sizeof(wchar_t));
				m_buf[length] = L'\0';
			}
			else
				m_buf = const_cast<wchar_t*>(sz);

			m_len = length;
		}

		StringNode(const wchar_t* sz1, size_t len1, const wchar_t* sz2, size_t len2) : m_buf(0), m_len(0), m_own(true), m_fs(0), m_refcount(1)
		{
			assert(sz1);
			assert(len1);
			assert(sz2);
			assert(len2);

			OMEGA_NEW(m_buf,wchar_t[len1+len2+1]);
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
			
			StringNode* pNode;
			OMEGA_NEW(pNode,StringNode(m_buf,m_len,true));
			return pNode;
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
				uint32_t     index;
				int32_t      alignment;
				std::wstring format;
				std::wstring suffix;
			};
			std::list<insert_t> m_listInserts;
			size_t              m_curr_arg;
			std::wstring        m_prefix;
		};
		format_state_t* m_fs;

		void parse_format();

	private:
		OOBase::AtomicInt<size_t> m_refcount;

		~StringNode()
		{
			if (m_own)
				delete [] m_buf;
			delete m_fs;
		}

		size_t find_brace(size_t start, wchar_t brace);
		void merge_braces(std::wstring& str);
		void parse_arg(size_t& pos);
	};

	std::wstring align(const std::wstring& str, int align)
	{
		unsigned width = (align < 0 ? -align : align);
		if (str.size() >= width)
			return str;

		if (align < 0)
			return str + std::wstring(width-str.size(),L' ');
		else
			return std::wstring(width-str.size(),L' ') + str;
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

	StringNode* pNode = 0;
	OMEGA_NEW(pNode,StringNode(wlen));
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

	StringNode* pNode = 0;
	OMEGA_NEW(pNode,StringNode(wsz,length,copy==0 ? false : true));
	return pNode;
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

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_add1,2,((in),void*,s1,(in),const void*,s2))
{
	StringNode* pOrig = static_cast<StringNode*>(s1);
	const StringNode* pAdd = static_cast<const StringNode*>(s2);

	if (!pOrig)
		return pAdd->Own();
	
	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(pOrig->m_buf,pOrig->m_len,pAdd->m_buf,pAdd->m_len));
	
	pOrig->Release();
	
	return pNode;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_add2,2,((in),void*,s1,(in),wchar_t,c))
{
	StringNode* pOrig = static_cast<StringNode*>(s1);

	if (!pOrig)
		return OOCore_string_t__ctor2(&c,1,1);

	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(pOrig->m_buf,pOrig->m_len,&c,1));

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

	StringNode* s2 = 0;
	OMEGA_NEW(s2,StringNode(static_cast<const StringNode*>(s1)->m_buf,static_cast<const StringNode*>(s1)->m_len,true));

	for (wchar_t* p=s2->m_buf;size_t(p-s2->m_buf) < s2->m_len;++p)
		*p = towlower(*p);

	return s2;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_toupper,1,((in),const void*,s1))
{
	if (!s1)
		return 0;

	StringNode* s2 = 0;
	OMEGA_NEW(s2,StringNode(static_cast<const StringNode*>(s1)->m_buf,static_cast<const StringNode*>(s1)->m_len,true));

	for (wchar_t* p=s2->m_buf;size_t(p-s2->m_buf) < s2->m_len;++p)
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
		for (;towlower(*p) != ci && size_t(p-st)<len;++p)
			;
	}
	else
	{
		for (;*p != c && size_t(p-st)<len;++p)
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
		for (;towlower(*p) == ci && size_t(p-st)<len;++p)
			;
	}
	else
	{
		for (;*p == c && size_t(p-st)<len;++p)
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

	for (;OOCore_string_t_find1_Impl(s2,*p,0,bIgnoreCase) == string_t::npos && size_t(p-st)<len;++p)
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

	for (;OOCore_string_t_find1_Impl(s2,*p,0,bIgnoreCase) != string_t::npos && size_t(p-st)<len;++p)
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
		for (;towlower(*p) != ci && p>=st;--p)
			;
	}
	else
	{
		for (;*p != c && p>=st;--p)
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

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(s->m_buf,length,true));
	return s2;
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

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(s->m_buf + start,length,true));
	return s2;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_right,2,((in),const void*,s1,(in),size_t,length))
{
	const StringNode* s = static_cast<const StringNode*>(s1);
	if (length >= s->m_len)
		return s->Own();

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(s->m_buf + s->m_len-length,length,true));
	return s2;
}

void StringNode::parse_arg(size_t& pos)
{
	size_t end = find_brace(pos,L'}');
	if (end == string_t::npos)
		throw Formatting::IFormattingException::Create(L"Missing matching } in format string",OMEGA_SOURCE_INFO);

	size_t comma = OOCore_string_t_find1_Impl(this,L',',pos,0);
	size_t colon = OOCore_string_t_find1_Impl(this,L':',pos,0);
	if (comma == pos || colon == pos)
		throw Formatting::IFormattingException::Create(L"Missing index in format string",OMEGA_SOURCE_INFO);

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
		ins.format.assign(m_buf+colon+1,end-colon-1);
		merge_braces(ins.format);
	}

	m_fs->m_listInserts.push_back(ins);

	pos = end + 1;
}

void StringNode::merge_braces(std::wstring& str)
{
	for (size_t pos = 0;;)
	{
		size_t found = str.find(L'{',pos);
		if (found == std::wstring::npos)
			break;

		str.erase(found,1);
		pos = found + 1;
	}

	for (size_t pos = 0;;)
	{
		size_t found = str.find(L'}',pos);
		if (found == std::wstring::npos)
			break;

		str.erase(found,1);
		pos = found + 1;
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
		throw Formatting::IFormattingException::Create(L"No inserts in format string",OMEGA_SOURCE_INFO);

	OMEGA_NEW(m_fs,format_state_t);
	m_fs->m_curr_arg = (size_t)-1;
	m_fs->m_prefix.assign(m_buf,pos++);
	merge_braces(m_fs->m_prefix);

	// Parse args
	for (;;)
	{
		parse_arg(pos);

		size_t found = find_brace(pos,L'{');

		std::wstring suffix;
		if (found == string_t::npos)
			suffix.assign(m_buf+pos);
		else
			suffix.assign(m_buf+pos,found-pos);

		merge_braces(suffix);

		m_fs->m_listInserts.back().suffix = suffix;

		if (found == string_t::npos)
			break;

		pos = found + 1;
	}

	for (size_t idx = m_fs->m_curr_arg;;)
	{
		size_t count = 0;
		bool bFound = false;
		for (std::list<StringNode::format_state_t::insert_t>::const_iterator i=m_fs->m_listInserts.begin();i!=m_fs->m_listInserts.end();++i)
		{
			if (i->index <= idx)
				++count;

			if (i->index == idx)
			{
				bFound = true;
				++idx;
				break;
			}
		}

		if (!bFound)
		{
			if (count < m_fs->m_listInserts.size())
				throw Formatting::IFormattingException::Create(L"Index gap in format string",OMEGA_SOURCE_INFO);
			break;
		}
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_get_arg,3,((in),size_t,idx,(in_out),void**,s1,(out),void**,fmt))
{
	StringNode* s = static_cast<StringNode*>(*s1);
	if (!s)
		throw Formatting::IFormattingException::Create(L"Empty format string",OMEGA_SOURCE_INFO);

	try
	{
		size_t arg;
		if (!idx)
		{
			// Clone s
			StringNode* pNewNode = 0;
			OMEGA_NEW(pNewNode,StringNode(s->m_buf,s->m_len,true));

			if (s->m_fs)
			{
				OMEGA_NEW(pNewNode->m_fs,StringNode::format_state_t(*s->m_fs));
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
			arg = s->m_fs->m_curr_arg-1;
		}

		for (std::list<StringNode::format_state_t::insert_t>::const_iterator i=s->m_fs->m_listInserts.begin();i!=s->m_fs->m_listInserts.end();++i)
		{
			if (i->index == arg)
			{
				*fmt = OOCore_string_t__ctor2_Impl(i->format.data(),i->format.size(),1);
				return 1;
			}
		}

		// Now measure how much space we need
		std::wstring str = s->m_fs->m_prefix;
		for (std::list<StringNode::format_state_t::insert_t>::const_iterator i=s->m_fs->m_listInserts.begin();i!=s->m_fs->m_listInserts.end();++i)
		{
			str += i->format;
			str += i->suffix;
		}

		if (str.size() > s->m_len)
		{
			wchar_t* buf_new = 0;
			OMEGA_NEW(buf_new,wchar_t[str.size()+1]);
			buf_new[str.size()] = L'\0';

			if (s->m_own)
				delete [] s->m_buf;
			s->m_buf = buf_new;
		}

		memcpy(s->m_buf,str.data(),str.size()*sizeof(wchar_t));
		s->m_buf[str.size()] = L'\0';
		s->m_len = str.size();
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	return 0;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_set_arg,2,((in),void*,s1,(in),void*,arg))
{
	StringNode* s = static_cast<StringNode*>(s1);

	try
	{
		for (std::list<StringNode::format_state_t::insert_t>::iterator i=s->m_fs->m_listInserts.begin();i!=s->m_fs->m_listInserts.end();++i)
		{
			if (i->index == s->m_fs->m_curr_arg-1)
			{
				if (i == s->m_fs->m_listInserts.begin())
				{
					std::wstring str;
					if (arg)
						str.assign(static_cast<StringNode*>(arg)->m_buf,static_cast<StringNode*>(arg)->m_len);

					s->m_fs->m_prefix += align(str,i->alignment);
					s->m_fs->m_prefix += i->suffix;
					s->m_fs->m_listInserts.pop_front();

					while (!s->m_fs->m_listInserts.empty())
					{
						i = s->m_fs->m_listInserts.begin();
						if (i->index == (unsigned int)-1)
						{
							s->m_fs->m_prefix += align(i->format,i->alignment);
							s->m_fs->m_prefix += i->suffix;
							s->m_fs->m_listInserts.pop_front();
						}
						else
							break;
					}
				}
				else
				{
					i->format.empty();
					if (arg)
						i->format.assign(static_cast<StringNode*>(arg)->m_buf,static_cast<StringNode*>(arg)->m_len);

					i->index = (unsigned int)-1;
				}
				break;
			}
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
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

	std::ostringstream ss;
	ss.imbue(std::locale::classic());

	ss.setf(std::ios_base::hex,std::ios_base::basefield);
	ss.setf(std::ios_base::uppercase);
	ss.fill('0');

	ss << '{';
	ss << std::setw(8) << guid.Data1 << '-';
	ss << std::setw(4) << guid.Data2 << '-';
	ss << std::setw(4) << guid.Data3 << '-';

	ss << std::setw(2) << static_cast<unsigned int>(guid.Data4[0]);
	ss << std::setw(2) << static_cast<unsigned int>(guid.Data4[1]);
	ss << '-';

	ss << std::setw(2) << static_cast<unsigned int>(guid.Data4[2]);
	ss << std::setw(2) << static_cast<unsigned int>(guid.Data4[3]);
	ss << std::setw(2) << static_cast<unsigned int>(guid.Data4[4]);
	ss << std::setw(2) << static_cast<unsigned int>(guid.Data4[5]);
	ss << std::setw(2) << static_cast<unsigned int>(guid.Data4[6]);
	ss << std::setw(2) << static_cast<unsigned int>(guid.Data4[7]);
	ss << '}';

	return string_t(ss.str().c_str(),true);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,OOCore_guid_t_from_string,2,((in),const wchar_t*,sz,(out),guid_t&,result))
{
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
