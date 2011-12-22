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

#if defined(HAVE_UUID_UUID_H)
#include <uuid/uuid.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <sys/stat.h>
#include <fcntl.h>
#endif

using namespace Omega;

namespace
{
	bool priv_isxdigit(char c)
	{
		return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
	}

	class StringNode
	{
		enum Flags
		{
			eConst = 0,
			eLocal = 1,
			eHeap = 2
		};

	public:
		StringNode() :
			m_refcount(1), m_flags(eLocal), m_len(0)
		{
			m_u.m_local_buffer[0] = '\0';
		}

		StringNode(const char* sz, size_t len) :
			m_refcount(1), m_flags(eConst), m_len(len)
		{
			m_u.m_const_buffer = sz;
		}

		~StringNode()
		{
			if (m_flags & eHeap)
				OOBase::HeapAllocator::free(m_u.m_alloc_buffer);
		}

		char* grow(size_t extra, int& err);

		const char* buffer() const
		{
			switch (m_flags)
			{
			case eLocal:
				return m_u.m_local_buffer;
			case eHeap:
				return m_u.m_alloc_buffer;
			default:
				return m_u.m_const_buffer;
			}
		}

		size_t length() const
		{
			return m_len;
		}

		StringNode* addref() const
		{
			++m_refcount;
			return const_cast<StringNode*>(this);
		}

		void release()
		{
			if (--m_refcount == 0)
				delete this;
		}

	private:
		mutable OOBase::Atomic<size_t> m_refcount;
		unsigned int                   m_flags;
		size_t                         m_len;
		union U
		{
			char*       m_alloc_buffer;
			char        m_local_buffer[24];
			const char* m_const_buffer;
		} m_u;
	};

	template <typename T>
	bool any_compare(const any_t& lhs, const any_t& rhs)
	{
		T v1,v2;
		if (lhs.Coerce(v1) != any_t::castValid || rhs.Coerce(v2) != any_t::castValid)
			return false;

		return (v1 == v2);
	}

	int compare(const char* sz1, size_t len1, const char* sz2, size_t len2, size_t pos, size_t len)
	{
		if (len < len1)
			len1 = len;

		if (pos >= len1)
			len1 = 0;
		else
			len1 = len1 - pos;

		len = (len1 > len2 ? len2 : len1);
		if (len)
		{
			int r = memcmp(sz1+pos,sz2,len);
			if (r != 0)
				return r;
		}

		if (len2 < len1)
			return 1;
		else if (len1 < len2)
			return -1;

		return 0;
	}
}

char* StringNode::grow(size_t extra, int& err)
{
	err = 0;
	char* buffer = NULL;
	if (m_flags != eConst)
	{
		if (++m_refcount == 2)
		{
			// We have only one reference, so we can grow
			if (m_flags == eLocal)
			{
				if (m_len + extra < sizeof(m_u.m_local_buffer))
					buffer = m_u.m_local_buffer + m_len;
				else
				{
					char local_buffer[sizeof(m_u.m_local_buffer)] = {0};
					if (m_len)
						memcpy(local_buffer,m_u.m_local_buffer,m_len);

					m_u.m_alloc_buffer = static_cast<char*>(OOBase::HeapAllocator::allocate(m_len + extra + 1));
					if (!m_u.m_alloc_buffer)
						err = ERROR_OUTOFMEMORY;
					else
					{
						m_flags = eHeap;

						if (m_len)
							memcpy(m_u.m_alloc_buffer,local_buffer,m_len);

						buffer = m_u.m_alloc_buffer + m_len;
					}
				}
			}
			else
			{
				// Reallocate
				char* new_buffer = static_cast<char*>(OOBase::HeapAllocator::reallocate(m_u.m_alloc_buffer,m_len + extra + 1));
				if (!new_buffer)
					err = ERROR_OUTOFMEMORY;
				else
				{
					m_u.m_alloc_buffer = new_buffer;
					buffer = m_u.m_alloc_buffer + m_len;
				}
			}

			if (buffer)
			{
				m_len += extra;
				buffer[extra] = '\0';
			}
		}
		--m_refcount;
	}

	return buffer;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t__ctor,2,((in),const char*,sz,(in),size_t,len))
{
	if (sz && len == string_t::npos)
		len = strlen(sz);

	if (!sz || !len)
		return NULL;

	StringNode* pNode = new (OOCore::throwing) StringNode();
	int err = 0;
	char* buffer = pNode->grow(len,err);
	if (!buffer || err != 0)
	{
		delete pNode;
		OMEGA_THROW(err ? err : ERROR_OUTOFMEMORY);
	}

	memcpy(buffer,sz,len);

	return pNode;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t__const_ctor,2,((in),const char*,sz,(in),size_t,len))
{
	if (!sz || !len)
		return NULL;

	return new (OOCore::throwing) StringNode(sz,len);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_addref,1,((in),void*,s1))
{
	if (s1)
		static_cast<StringNode*>(s1)->addref();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_release,1,((in),void*,s1))
{
	if (s1)
		static_cast<StringNode*>(s1)->release();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_assign,2,((in),void*,s1,(in),const void*,s2))
{
	if (s2)
		static_cast<const StringNode*>(s2)->addref();

	if (s1)
		static_cast<StringNode*>(s1)->release();

	return const_cast<void*>(s2);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const char*,OOCore_string_t_cast,2,((in),const void*,s1,(in),size_t*,plen))
{
	const char* buffer = NULL;
	if (s1)
	{
		if (plen)
			*plen = static_cast<const StringNode*>(s1)->length();

		buffer = static_cast<const StringNode*>(s1)->buffer();
	}

	if (!buffer)
	{
		buffer = "";
		if (plen)
			*plen = 0;
	}

	return buffer;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_append1,2,((in),void*,s1,(in),const void*,s2))
{
	if (!s2)
		return s1;

	const StringNode* pAdd = static_cast<const StringNode*>(s2);
	StringNode* pOrig = static_cast<StringNode*>(s1);
	if (!pOrig)
		return pAdd->addref();

	size_t add_len = pAdd->length();

	int err = 0;
	char* buffer = pOrig->grow(add_len,err);
	if (err != 0)
		OMEGA_THROW(err);

	if (buffer)
	{
		memcpy(buffer,pAdd->buffer(),add_len);
		return pOrig;
	}
	else
	{
		size_t orig_len = pOrig->length();
		StringNode* pNode = new (OOCore::throwing) StringNode();
		buffer = pNode->grow(orig_len + add_len,err);
		if (!buffer || err != 0)
		{
			delete pNode;
			OMEGA_THROW(err ? err : ERROR_OUTOFMEMORY);
		}

		memcpy(buffer,pOrig->buffer(),orig_len);
		memcpy(buffer+orig_len,pAdd->buffer(),add_len);

		pOrig->release();
		return pNode;
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_append2,3,((in),void*,s1,(in),const char*,sz,(in),size_t,len))
{
	if (len == string_t::npos)
		len = strlen(sz);

	if (len == 0)
		return s1;

	if (!s1)
		return OOCore_string_t__ctor(sz,len);

	StringNode* pOrig = static_cast<StringNode*>(s1);

	int err = 0;
	char* buffer = pOrig->grow(len,err);
	if (err != 0)
		OMEGA_THROW(err);

	if (buffer)
	{
		memcpy(buffer,sz,len);
		return pOrig;
	}
	else
	{
		size_t orig_len = pOrig->length();
		StringNode* pNode = new (OOCore::throwing) StringNode();
		buffer = pNode->grow(orig_len + len,err);
		if (!buffer || err != 0)
		{
			delete pNode;
			OMEGA_THROW(err ? err : ERROR_OUTOFMEMORY);
		}

		memcpy(buffer,pOrig->buffer(),orig_len);
		memcpy(buffer+orig_len,sz,len);
		
		pOrig->release();
		return pNode;
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cmp1,4,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos,(in),size_t,length))
{
	const char* buf1 = NULL;
	const char* buf2 = NULL;
	size_t len1 = 0;
	size_t len2 = 0;

	if (s1)
	{
		const StringNode* str1 = static_cast<const StringNode*>(s1);
		buf1 = str1->buffer();
		len1 = str1->length();
	}

	if (s2)
	{
		const StringNode* str2 = static_cast<const StringNode*>(s2);
		buf2 = str2->buffer();
		len2 = str2->length();
	}

	return compare(buf1,len1,buf2,len2,pos,length);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cmp2,2,((in),const void*,s1,(in),const char*,sz))
{
	const char* buf = NULL;
	size_t len1 = 0;
	size_t len2 = 0;

	if (s1)
	{
		const StringNode* str1 = static_cast<const StringNode*>(s1);
		buf = str1->buffer();
		len1 = str1->length();
	}

	if (sz)
		len2 = strlen(sz);

	return compare(buf,len1,sz,len2,0,len1);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find1,3,((in),const void*,s1,(in),char,c,(in),size_t,pos))
{
	if (!s1)
		return string_t::npos;

	const StringNode* str1 = static_cast<const StringNode*>(s1);
	size_t len = str1->length();
	if (pos >= len)
		len = 0;
	else
		len -= pos;

	if (len == 0)
		return string_t::npos;

	const char* start = str1->buffer();
	const char* found = static_cast<const char*>(memchr(start + pos,c,len));
	if (!found)
		return string_t::npos;

	return static_cast<size_t>(found - start);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find2,3,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos))
{
	if (!s1)
		return string_t::npos;

	if (!s2)
		return 0;

	const StringNode* str1 = static_cast<const StringNode*>(s1);
	const StringNode* str2 = static_cast<const StringNode*>(s2);

	size_t len = str1->length();
	if (pos >= len)
		len = 0;
	else
		len -= pos;

	if (len == 0)
		return string_t::npos;

	size_t search_len = str2->length();
	if (search_len > len)
		return string_t::npos;

	const char* search = str2->buffer();
	const char* start = str1->buffer() + pos;

	for (size_t offset = pos;len != 0;start += offset + 1)
	{
		const char* found = static_cast<const char*>(memchr(start,search[0],len));
		if (!found)
			return string_t::npos;

		size_t p = static_cast<size_t>(found - start);
		len -= p;

		if (search_len > len)
			return string_t::npos;

		offset += p;

		if (memcmp(found,search,search_len) == 0)
			return offset;
	}

	return string_t::npos;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_not,3,((in),const void*,s1,(in),char,c,(in),size_t,pos))
{
	if (!s1)
		return string_t::npos;

	const StringNode* str1 = static_cast<const StringNode*>(s1);
	
	size_t len = str1->length();
	const char* start = str1->buffer() + pos;
	
	for (size_t i=pos;i < len;++i)
	{
		if (*(start++) != c)
			return i;
	}
	
	return string_t::npos;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_oneof,3,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos))
{
	if (!s1 || !s2)
		return string_t::npos;

	const StringNode* str1 = static_cast<const StringNode*>(s1);
	const StringNode* str2 = static_cast<const StringNode*>(s2);

	size_t len = str1->length();
	if (pos >= len)
		len = 0;
	else
		len -= pos;

	if (len == 0)
		return string_t::npos;

	const char* search = str2->buffer();
	const char* start = str1->buffer() + pos;

	for (size_t offset = pos;len != 0;start += offset + 1)
	{
		const char* found = strpbrk(start,search);
		if (found)
			return offset + static_cast<size_t>(found - start);

		size_t p = strlen(start);
		offset += p;
		len -= p;
	}

	return string_t::npos;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_notof,3,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos))
{
	if (!s1 || !s2)
		return string_t::npos;

	const StringNode* str1 = static_cast<const StringNode*>(s1);
	const StringNode* str2 = static_cast<const StringNode*>(s2);

	size_t len = str1->length();
	if (pos >= len)
		len = 0;
	else
		len -= pos;

	if (len == 0)
		return string_t::npos;

	const char* search = str2->buffer();
	const char* start = str1->buffer() + pos;

	for (size_t offset = pos;len != 0;start += offset + 1)
	{
		size_t p = strlen(start);
		size_t f = strspn(start,search);
		if (f < p)
			return offset + f;

		offset += p;
		len -= p;
	}

	return string_t::npos;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_rfind,3,((in),const void*,s1,(in),char,c,(in),size_t,pos))
{
	if (!s1)
		return string_t::npos;

	const StringNode* str1 = static_cast<const StringNode*>(s1);
	size_t len = str1->length();
	if (pos >= len)
		pos = len-1;
	
	if (len > 0)
	{
		const char* start = str1->buffer() + pos;
		for (size_t i=pos+1;i > 0;--i)
		{
			if (*(start--) == c)
				return i-1;
		}
	}

	return string_t::npos;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_left,2,((in),const void*,s1,(in),size_t,length))
{
	if (!s1 || !length)
		return NULL;

	const StringNode* str1 = static_cast<const StringNode*>(s1);
	if (str1->length() <= length)
		return str1->addref();

	return OOCore_string_t__ctor(str1->buffer(),length);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_mid,3,((in),const void*,s1,(in),size_t,start,(in),size_t,length))
{
	if (!s1 || !length)
		return NULL;

	const StringNode* str1 = static_cast<const StringNode*>(s1);

	if (start > str1->length())
		return NULL;

	if (length > str1->length() - start)
		length = str1->length() - start;

	return OOCore_string_t__ctor(str1->buffer()+start,length);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_string_t_right,2,((in),const void*,s1,(in),size_t,length))
{
	if (!s1 || !length)
		return NULL;

	const StringNode* str1 = static_cast<const StringNode*>(s1);
	if (str1->length() <= length)
		return str1->addref();

	return OOCore_string_t__ctor(str1->buffer()+(str1->length() - length),length);
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

	return str.c_str();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_guid_t_from_string,2,((in),const char*,sz,(in_out),guid_base_t*,result))
{
	// Do this manually...
	result->Data1 = 0;
	result->Data2 = 0;
	result->Data3 = 0;
	memset(result->Data4,sizeof(result->Data4),0);
	
	if (!sz || sz[0] != '{' || !priv_isxdigit(sz[1]))
		return 0;

	const char* endp = NULL;
	result->Data1 = OOCore::strtoul(sz+1,endp,16);
	if (endp != sz+9)
		return 0;

	if (sz[9] != '-' || !priv_isxdigit(sz[10]))
		return 0;

	result->Data2 = static_cast<uint16_t>(OOCore::strtoul(sz+10,endp,16));
	if (endp != sz+14 || sz[14] != '-' || !priv_isxdigit(sz[15]))
		return 0;

	result->Data3 = static_cast<uint16_t>(OOCore::strtoul(sz+15,endp,16));
	if (endp != sz+19 || sz[19] != '-' || !priv_isxdigit(sz[20]))
		return 0;

	uint32_t v1 = OOCore::strtoul(sz+20,endp,16);
	if (endp != sz+24)
		return 0;

	result->Data4[0] = static_cast<byte_t>((v1 >> 8) & 0xFF);
	result->Data4[1] = static_cast<byte_t>(v1 & 0xFF);

	if (sz[24] != '-' && !priv_isxdigit(sz[25]))
		return 0;

	uint64_t v2 = OOCore::strtou64(sz+25,endp,16);
	if (endp != sz+37)
		return 0;

	result->Data4[2] = static_cast<byte_t>(((v2 >> 32) >> 8) & 0xFF);
	result->Data4[3] = static_cast<byte_t>((v2 >> 32) & 0xFF);
	result->Data4[4] = static_cast<byte_t>((v2 >> 24) & 0xFF);
	result->Data4[5] = static_cast<byte_t>((v2 >> 16) & 0xFF);
	result->Data4[6] = static_cast<byte_t>((v2 >> 8) & 0xFF);
	result->Data4[7] = static_cast<byte_t>(v2 & 0xFF);

	if (sz[37] != '}' || sz[38] != '\0')
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

#elif defined(HAVE_UNISTD_H)

	int fd = open("/dev/urandom",O_RDONLY);
	if (fd == -1)
		fd = open("/dev/random",O_RDONLY);

	if (fd == -1)
		OMEGA_THROW(errno);

	guid_base_t res;
	if (read(fd,&res,sizeof(res)) != sizeof(res))
	{
		int err = errno;
		close(fd);
		OMEGA_THROW(err);
	}

	close(fd);
	return res;

#else
#error Need to implement uuid creation on your platform
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
