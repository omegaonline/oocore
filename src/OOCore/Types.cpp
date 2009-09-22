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
		StringNode(size_t length) : m_buf(0), m_len(0), m_fs(0), m_refcount(1)
		{
			assert(length);

			OMEGA_NEW(m_buf,wchar_t[length+1]);
			m_buf[length] = L'\0';
			m_len = length;
		}

		StringNode(const wchar_t* sz, size_t length) : m_buf(0), m_len(0), m_fs(0), m_refcount(1)
		{
			assert(sz);
			assert(length);

			OMEGA_NEW(m_buf,wchar_t[length+1]);
			memcpy(m_buf,sz,length*sizeof(wchar_t));
			m_buf[length] = L'\0';
			m_len = length;
		}

		StringNode(const wchar_t* sz1, size_t len1, const wchar_t* sz2, size_t len2) : m_buf(0), m_len(0), m_fs(0), m_refcount(1)
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

		void Release()
		{
			if (--m_refcount==0)
				delete this;
		}

		wchar_t* m_buf;
		size_t   m_len;

		struct format_state_t
		{
			struct insert_t
			{
				unsigned int index;
				int          alignment;
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
			delete [] m_buf;
			delete m_fs;
		}

		size_t find_skip(size_t start);
		void merge_percent(std::wstring& str, size_t start, size_t end);
		void parse_arg(size_t& pos);
	};

	unsigned int parse_uint_hex(wchar_t c)
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

	unsigned int parse_uint(wchar_t c)
	{
		if (c >= L'0' && c <= L'9')
			return (c-L'0');
		else
			throw int(0);
	}

	unsigned int parse_uint(const wchar_t* sz)
	{
		unsigned int v = 0;
		const wchar_t* p = sz;
		if (*p == L'+')
			++p;

		try
		{
			for (;;)
			{
				unsigned int i = parse_uint(*p++);

				if (v > UINT_MAX/10)
					break;
				v *= 10;

				if (v > UINT_MAX-i)
					break;
				v += i;		
			}
		}
		catch (int)
		{}

		return v;
	}

	int parse_int(const wchar_t* sz)
	{
		bool bNeg = false;
		int v = 0;
		const wchar_t* p = sz;
		if (*p == L'+')
			++p;
		else if (*p == L'-')
		{
			++p;
			bNeg = true;
		}

		try
		{
			for (;;)
			{
				unsigned int i = parse_uint(*p++);

				if (v > INT_MAX/10)
					break;
				v *= 10;

				if ((unsigned int)v > INT_MAX-i)
					break;
				v += i;		
			}
		}
		catch (int)
		{}

		return (bNeg ? -v : v);
	}

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

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor2,1,((in),const void*,s1))
{
	return static_cast<const StringNode*>(s1)->AddRef();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor3,2,((in),const wchar_t*,wsz,(in),size_t,length))
{
	if (length == Omega::string_t::npos)
		length = wcslen(wsz);

	if (!length)
		return 0;

	StringNode* pNode = 0;
	OMEGA_NEW(pNode,StringNode(wsz,length));
	return pNode;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t__dctor,1,((in),void*,s1))
{
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

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_assign2,2,((in),void*,s1,(in),const wchar_t*,wsz))
{
	if (s1)
		static_cast<StringNode*>(s1)->Release();

	if (!wsz)
		return 0;

	size_t length = wcslen(wsz);
	if (length == 0)
		return 0;

	StringNode* pNode = 0;
	OMEGA_NEW(pNode,StringNode(wsz,length));
	return pNode;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const wchar_t*,OOCore_string_t_cast,1,((in),const void*,s1))
{
	if (!s1)
		return L"";

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

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_add,2,((in),void*,s1,(in),const void*,s2))
{
	StringNode* pOrig = static_cast<StringNode*>(s1);
	const StringNode* pAdd = static_cast<const StringNode*>(s2);

	if (!pOrig)
		return pAdd->AddRef();

	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(pOrig->m_buf,pOrig->m_len,pAdd->m_buf,pAdd->m_len));

	pOrig->Release();

	return pNode;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cmp1,5,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos,(in),size_t,length,(in),int,bIgnoreCase))
{
	const StringNode* s = static_cast<const StringNode*>(s1);
	if (length > s->m_len - pos)
		length = s->m_len - pos;

	size_t len2 = static_cast<const StringNode*>(s2)->m_len;

	const wchar_t* st1 = s->m_buf + pos;
	const wchar_t* st2 = static_cast<const StringNode*>(s2)->m_buf;
	
	const wchar_t* p1 = st1;
	const wchar_t* p2 = st2;

	wint_t l1 = 0, l2 = 0;
	if (bIgnoreCase)
	{
		while ((size_t(p1-st1)<length) && (size_t(p2-st2)<len2) && (l1 = towlower(*p1)) == (l2 = towlower(*p2)))
		{
			++p1;
			++p2;
		}
	}
	else
	{
		while ((size_t(p1-st1)<length) && (size_t(p2-st2)<len2) && (l1 = *p1) == (l2 = *p2))
		{
			++p1;
			++p2;
		}
	}

	if (l1 != l2)
		return (l1 < l2 ? -1 : 1);

	if (length != len2)
		return (length < len2 ? -1 : 1);

	return 0;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cmp2,5,((in),const void*,s1,(in),const wchar_t*,st2,(in),size_t,pos,(in),size_t,length,(in),int,bIgnoreCase))
{
	const StringNode* s = static_cast<const StringNode*>(s1);
	if (length > s->m_len - pos)
		length = s->m_len - pos;

	const wchar_t* st1 = s->m_buf + pos;
		
	const wchar_t* p1 = st1;
	const wchar_t* p2 = st2;

	wint_t l1 = 0, l2 = 0;
	if (bIgnoreCase)
	{
		while ((size_t(p1-st1)<length) && (*p2 != L'\0') && (l1 = towlower(*p1)) == (l2 = towlower(*p2)))
		{
			++p1;
			++p2;
		}
	}
	else
	{
		while ((size_t(p1-st1)<length) && (*p2 != L'\0') && (l1 = *p1) == (l2 = *p2))
		{
			++p1;
			++p2;
		}
	}

	if (l1 != l2)
		return (l1 < l2 ? -1 : 1);

	size_t len2 = size_t(p2-st2);
	if (length != len2)
		return (length < len2 ? -1 : 1);

	return 0;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_tolower,1,((in),const void*,s1))
{
	if (!s1)
		return 0;

	StringNode* s2 = 0;
	OMEGA_NEW(s2,StringNode(static_cast<const StringNode*>(s1)->m_buf,static_cast<const StringNode*>(s1)->m_len));
	
	for (wchar_t* p=s2->m_buf;size_t(p-s2->m_buf) < s2->m_len;++p)
		*p = towlower(*p);

	return s2;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_toupper,1,((in),const void*,s1))
{
	if (!s1)
		return 0;

	StringNode* s2 = 0;
	OMEGA_NEW(s2,StringNode(static_cast<const StringNode*>(s1)->m_buf,static_cast<const StringNode*>(s1)->m_len));
	
	for (wchar_t* p=s2->m_buf;size_t(p-s2->m_buf) < s2->m_len;++p)
		*p = towupper(*p);

	return s2;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find1,4,((in),const void*,s1,(in),wchar_t,c,(in),size_t,pos,(in),int,bIgnoreCase))
{
	size_t len = static_cast<const StringNode*>(s1)->m_len;
	if (pos >= len)
		return Omega::string_t::npos;

	const wchar_t* st = static_cast<const StringNode*>(s1)->m_buf;
	const wchar_t* p = st + pos;

	if (bIgnoreCase)
	{
		wint_t ci = towlower(c);
		for (;tolower(*p) != ci && size_t(p-st)<len;++p)
			;
	}
	else
	{
		for (;*p != c && size_t(p-st)<len;++p)
			;
	}

	if (size_t(p-st) == len)
		return Omega::string_t::npos;
	else
		return size_t(p-st);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_not,4,((in),const void*,s1,(in),wchar_t,c,(in),size_t,pos,(in),int,bIgnoreCase))
{
	size_t len = static_cast<const StringNode*>(s1)->m_len;
	if (pos >= len)
		return Omega::string_t::npos;

	const wchar_t* st = static_cast<const StringNode*>(s1)->m_buf;
	const wchar_t* p = st + pos;

	if (bIgnoreCase)
	{
		wint_t ci = towlower(c);
		for (;tolower(*p) == ci && size_t(p-st)<len;++p)
			;
	}
	else
	{
		for (;*p == c && size_t(p-st)<len;++p)
			;
	}

	if (size_t(p-st) == len)
		return Omega::string_t::npos;
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
		if (start == Omega::string_t::npos)
			break;

		if (OOCore_string_t_cmp1_Impl(s1,s2,start,len,bIgnoreCase) == 0)
			return start;

		pos = start + 1;
	}

	return Omega::string_t::npos;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_oneof,4,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos,(in),int,bIgnoreCase))
{
	size_t len = static_cast<const StringNode*>(s1)->m_len;
	if (pos >= len)
		return Omega::string_t::npos;

	const wchar_t* st = static_cast<const StringNode*>(s1)->m_buf;
	const wchar_t* p = st + pos;

	for (;OOCore_string_t_find1_Impl(s2,*p,0,bIgnoreCase) == Omega::string_t::npos && size_t(p-st)<len;++p)
		;
	
	if (size_t(p-st) == len)
		return Omega::string_t::npos;
	else
		return size_t(p-st);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_notof,4,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos,(in),int,bIgnoreCase))
{
	size_t len = static_cast<const StringNode*>(s1)->m_len;
	if (pos >= len)
		return Omega::string_t::npos;

	const wchar_t* st = static_cast<const StringNode*>(s1)->m_buf;
	const wchar_t* p = st + pos;

	for (;OOCore_string_t_find1_Impl(s2,*p,0,bIgnoreCase) != Omega::string_t::npos && size_t(p-st)<len;++p)
		;
	
	if (size_t(p-st) == len)
		return Omega::string_t::npos;
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
		for (;tolower(*p) != ci && p>=st;--p)
			;
	}
	else
	{
		for (;*p != c && p>=st;--p)
			;
	}

	if (p < st)
		return Omega::string_t::npos;
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
		return s->AddRef();

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(s->m_buf,length));
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
		return s->AddRef();

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(s->m_buf + start,length));
	return s2;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_right,2,((in),const void*,s1,(in),size_t,length))
{
	const StringNode* s = static_cast<const StringNode*>(s1);
	if (length >= s->m_len)
		return s->AddRef();

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(s->m_buf + s->m_len-length,length));
	return s2;
}

void StringNode::parse_arg(size_t& pos)
{
	size_t end = OOCore_string_t_find1_Impl(this,L'%',pos,0);
	if (end == Omega::string_t::npos)
		OMEGA_THROW(L"Missing matching % in format string");
	
	size_t comma = OOCore_string_t_find1_Impl(this,L',',pos,0);
	size_t colon = OOCore_string_t_find1_Impl(this,L':',pos,0);
	
	if (comma == pos || colon == pos)
		OMEGA_THROW(L"Missing index in format string");

	format_state_t::insert_t ins;
	ins.index = parse_uint(m_buf+pos);
	if (ins.index < m_fs->m_curr_arg)
		m_fs->m_curr_arg = ins.index;

	if (comma >= end)
	{
		ins.alignment = 0;
		comma = end;
	}
	else
	{
		++comma;
		ins.alignment = parse_int(m_buf+comma);
	}
	
	if (colon >= end)
		colon = end;
	else
		++colon;

	ins.format.assign(m_buf+colon,comma > colon ? comma-colon : end-colon);
	
	m_fs->m_listInserts.push_back(ins);

	pos = end + 1;
}

void StringNode::merge_percent(std::wstring& str, size_t start, size_t end)
{
	for (size_t pos = start;;)
	{
		size_t found = OOCore_string_t_find1_Impl(this,L'%',pos,0);
		if (found >= end)
		{
			if (end == Omega::string_t::npos)
				str.append(m_buf+pos,m_len-pos);
			else
				str.append(m_buf+pos,end-pos);
			break;
		}
				
		str.append(m_buf+pos,found-pos+1);
				
		// Skip %%
		start = pos + 2;
	}
}

size_t StringNode::find_skip(size_t start)
{
	for (;;)
	{
		size_t found = OOCore_string_t_find1_Impl(this,L'%',start,0);
		if (found == Omega::string_t::npos)
			return Omega::string_t::npos;
				
		if (found < m_len && m_buf[found+1] != L'%')
			return found;
				
		// Skip %%
		start = found + 2;
	}
}

void StringNode::parse_format()
{
	// Prefix first
	size_t pos = find_skip(0);
	if (pos == Omega::string_t::npos)
		return;

	OMEGA_NEW(m_fs,format_state_t);
	m_fs->m_curr_arg = (size_t)-1;
	m_fs->m_prefix.assign(m_buf,pos++);
			
	// Parse args
	for (;;)
	{
		parse_arg(pos);
		
		size_t found = find_skip(pos);
		merge_percent(m_fs->m_listInserts.back().suffix,pos,found);
		
		if (found == Omega::string_t::npos)
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
				OMEGA_THROW(L"Index gap in format string");
			break;
		}
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_get_arg,3,((in),size_t,idx,(in_out),void**,s1,(out),void**,fmt))
{
	StringNode* s = static_cast<StringNode*>(*s1);
	if (!s)
		OMEGA_THROW(L"Empty format string");
	
	try
	{
		size_t arg;
		if (!idx)
		{
			// Clone s
			StringNode* pNewNode = 0;
			OMEGA_NEW(pNewNode,StringNode(s->m_buf,s->m_len));

			if (s->m_fs)
				OMEGA_NEW(pNewNode->m_fs,StringNode::format_state_t(*s->m_fs));
			
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
				*fmt = OOCore_string_t__ctor3_Impl(i->format.data(),i->format.size());
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
						if (i->index == (size_t)-1)
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

					i->index = (size_t)-1;
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

// Forward declare the md5 stuff
extern "C"
{
	typedef char MD5Context[88];
	void MD5Init(MD5Context *pCtx);
	void MD5Update(MD5Context *pCtx, const unsigned char *buf, unsigned int len);
	void MD5Final(unsigned char digest[16], MD5Context *pCtx);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::string_t,OOCore_guid_t_to_string,2,((in),const Omega::guid_t&,guid,(in),const Omega::string_t&,strFormat))
{
#if defined(HAVE_UUID_UUID_H)

	char szBuf[38] = {0};
	uuid_unparse_upper(*(const uuid_t*)(&guid),szBuf);
	return Omega::string_t(szBuf,true);

#else

	wchar_t szBuf[38] = {0};

	swprintf(szBuf,L"%8.8lX-%4.4hX-%4.4hX-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
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
	
	return Omega::string_t(szBuf);

#endif
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,OOCore_guid_t_from_string,2,((in),const wchar_t*,sz,(out),Omega::guid_t&,result))
{
#if defined(HAVE_UUID_UUID_H)

	std::string str = OOBase::to_utf8(sz);
	const char* buf;
	if (str.length() == 38 && str[0] == '{')
	{
		if (str[37] != '}')
			return 0;

		buf = str.c_str() + 1;
	}
	else
	{
		if (str.length() == 36)
			return 0;

		buf = str.c_str();
	}

	uuid_t uuid;
	if (uuid_parse(buf,uuid))
		return 0;

	result = *(Omega::guid_t*)(uuid);
	return 1;

#else

	// Do this manually...
	result.Data1 = 0;
	result.Data2 = 0;
	result.Data3 = 0;
	memset(result.Data4,sizeof(result.Data4),0);

	bool bQuoted = false;
	if (sz[0] == L'{')
	{
		if (sz[37] != L'}')
			return 0;
		
		++sz;
		bQuoted = true;
	}

	try
	{
		unsigned int v = (parse_uint_hex(sz[0]) << 28);
		v += (parse_uint_hex(sz[1]) << 24);
		v += (parse_uint_hex(sz[2]) << 20);
		v += (parse_uint_hex(sz[3]) << 16);
		v += (parse_uint_hex(sz[4]) << 12);
		v += (parse_uint_hex(sz[5]) << 8);
		v += (parse_uint_hex(sz[6]) << 4);
		v += parse_uint_hex(sz[7]);
		result.Data1 = static_cast<Omega::uint32_t>(v);

		if (sz[8] != L'-')
			return 0;

		v = (parse_uint_hex(sz[9]) << 12);
		v += (parse_uint_hex(sz[10]) << 8);
		v += (parse_uint_hex(sz[11]) << 4);
		v += parse_uint_hex(sz[12]);
		result.Data2 = static_cast<Omega::uint16_t>(v);

		if (sz[13] != L'-')
			return 0;

		v = (parse_uint_hex(sz[14]) << 12);
		v += (parse_uint_hex(sz[15]) << 8);
		v += (parse_uint_hex(sz[16]) << 4);
		v += parse_uint_hex(sz[17]);
		result.Data3 = static_cast<Omega::uint16_t>(v);

		if (sz[18] != L'-')
			return 0;

		result.Data4[0] = static_cast<Omega::byte_t>((parse_uint_hex(sz[19]) << 4) + parse_uint_hex(sz[20]));
		result.Data4[1] = static_cast<Omega::byte_t>((parse_uint_hex(sz[21]) << 4) + parse_uint_hex(sz[22]));

		if (sz[23] != L'-')
			return false;

		result.Data4[2] = static_cast<Omega::byte_t>((parse_uint_hex(sz[24]) << 4) + parse_uint_hex(sz[25]));
		result.Data4[3] = static_cast<Omega::byte_t>((parse_uint_hex(sz[26]) << 4) + parse_uint_hex(sz[27]));
		result.Data4[4] = static_cast<Omega::byte_t>((parse_uint_hex(sz[28]) << 4) + parse_uint_hex(sz[29]));
		result.Data4[5] = static_cast<Omega::byte_t>((parse_uint_hex(sz[30]) << 4) + parse_uint_hex(sz[31]));
		result.Data4[6] = static_cast<Omega::byte_t>((parse_uint_hex(sz[32]) << 4) + parse_uint_hex(sz[33]));
		result.Data4[7] = static_cast<Omega::byte_t>((parse_uint_hex(sz[34]) << 4) + parse_uint_hex(sz[35]));

		if (bQuoted && sz[37] != L'\0')
			return 0;
		else if (!bQuoted && sz[36] != L'\0')
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

////////////////////////////////////////////////////
// Formatting starts here

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::string_t,OOCore_to_string_int_t,2,((in),Omega::int64_t,val,(in),const Omega::string_t&,strFormat))
{
	wchar_t sz[66];
	return Omega::string_t(_i64tow(val,sz,10));
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::string_t,OOCore_to_string_uint_t,2,((in),Omega::uint64_t,val,(in),const Omega::string_t&,strFormat))
{
	wchar_t sz[66];
	return Omega::string_t(_i64tow(val,sz,10));
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::string_t,OOCore_to_string_float_t,2,((in),Omega::float8_t,val,(in),const Omega::string_t&,strFormat))
{
	wchar_t sz[66];
	return Omega::string_t(_i64tow(val,sz,10));
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::string_t,OOCore_to_string_bool_t,2,((in),Omega::bool_t,val,(in),const Omega::string_t&,strFormat))
{
	wchar_t sz[66];
	return Omega::string_t(_i64tow(val,sz,10));
}
