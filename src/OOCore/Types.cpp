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

	struct string_handle;

	struct string_vtbl
	{
		char* (*grow)(void* p, size_t extra, int& err);
		const char* (*buffer)(const void* p);
		size_t (*length)(const void* p);
		void (*addref)(string_handle* p, int own);
		void (*release)(void* p);
	};

	struct string_handle
	{
		const string_vtbl* m_vtbl;
		void*              m_ptr;
	};

	class StringNode
	{
	public:
		StringNode() :
			m_refcount(1), m_len(0)
		{
			m_u.m_local_buffer[0] = '\0';
		}

		~StringNode()
		{
			if (m_len >= sizeof(m_u.m_local_buffer))
				OOBase::CrtAllocator::free(m_u.m_alloc_buffer);
		}

		char* grow(size_t extra, int& err);

		const char* buffer() const
		{
			if (m_len < sizeof(m_u.m_local_buffer))
				return m_u.m_local_buffer;

			return m_u.m_alloc_buffer;
		}

		size_t length() const
		{
			return m_len;
		}

		StringNode* addref()
		{
			++m_refcount;
			return this;
		}

		void release()
		{
			if (--m_refcount == 0)
				delete this;
		}

	private:
		OOBase::Atomic<size_t> m_refcount;
		size_t                 m_len;
		union U
		{
			char*       m_alloc_buffer;
			char        m_local_buffer[24];
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

	char* sn_grow(void* p, size_t extra, int& err) { return static_cast<StringNode*>(p)->grow(extra,err); }
	const char* sn_buffer(const void* p) { return static_cast<const StringNode*>(p)->buffer(); }
	size_t sn_length(const void* p) { return static_cast<const StringNode*>(p)->length(); }
	void sn_addref(string_handle* h, int) { static_cast<StringNode*>(h->m_ptr)->addref(); }
	void sn_release(void* p) { static_cast<StringNode*>(p)->release(); }

	const struct string_vtbl s_string_vtbl =
	{
		&sn_grow,
		&sn_buffer,
		&sn_length,
		&sn_addref,
		&sn_release
	};

	const char* const_buffer(const void* p) { return static_cast<const char*>(p); }
	size_t const_length(const void* p) { return strlen(static_cast<const char*>(p)); }

	void const_addref(string_handle* h, int own)
	{
		if (own)
			OOCore_string_t__ctor(h,static_cast<char*>(h->m_ptr),string_t::npos);
	}

	const struct string_vtbl s_const_string_vtbl =
	{
		NULL,
		&const_buffer,
		&const_length,
		&const_addref,
		NULL
	};

	char* string_grow(void* s, size_t extra, int& err)
	{
		err = 0;
		string_handle* h = static_cast<string_handle*>(s);
		if (h && h->m_ptr && h->m_vtbl && h->m_vtbl->grow)
			return (*h->m_vtbl->grow)(h->m_ptr,extra,err);
		return NULL;
	}
}

char* StringNode::grow(size_t extra, int& err)
{
	char* buffer = NULL;

	if (++m_refcount == 2)
	{
		// We have only one reference, so we can grow
		if (m_len < sizeof(m_u.m_local_buffer))
		{
			if (m_len + extra < sizeof(m_u.m_local_buffer))
				buffer = m_u.m_local_buffer + m_len;
			else
			{
				char local_buffer[sizeof(m_u.m_local_buffer)] = {0};
				if (m_len)
					memcpy(local_buffer,m_u.m_local_buffer,m_len);

				m_u.m_alloc_buffer = static_cast<char*>(OOBase::CrtAllocator::allocate(m_len + extra + 1));
				if (!m_u.m_alloc_buffer)
					err = ERROR_OUTOFMEMORY;
				else
				{
					if (m_len)
						memcpy(m_u.m_alloc_buffer,local_buffer,m_len);

					buffer = m_u.m_alloc_buffer + m_len;
				}
			}
		}
		else
		{
			// Reallocate
			char* new_buffer = static_cast<char*>(OOBase::CrtAllocator::reallocate(m_u.m_alloc_buffer,m_len + extra + 1));
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

	return buffer;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t__ctor,3,((in),void*,s,(in),const char*,sz,(in),size_t,len))
{
	string_handle* h = static_cast<string_handle*>(s);
	if (h)
	{
		if (sz && len == string_t::npos)
			len = strlen(sz);

		if (!sz || !len)
		{
			h->m_vtbl = NULL;
			h->m_ptr = NULL;
		}
		else
		{
			StringNode* pNode = new (OOCore::throwing) StringNode();
			int err = 0;
			char* buffer = pNode->grow(len,err);
			if (!buffer || err != 0)
			{
				delete pNode;
				OMEGA_THROW(err ? err : ERROR_OUTOFMEMORY);
			}

			memcpy(buffer,sz,len);

			h->m_vtbl = &s_string_vtbl;
			h->m_ptr = pNode;
		}
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t__const_ctor,3,((in),void*,s,(in),const char*,sz,(in),size_t,len))
{
	string_handle* h = static_cast<string_handle*>(s);
	if (h)
	{
		if (!sz || !len)
		{
			h->m_vtbl = NULL;
			h->m_ptr = NULL;
		}
		else
		{
			h->m_vtbl = &s_const_string_vtbl;
			h->m_ptr = const_cast<char*>(sz);
		}
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_addref,2,((in),void*,s,(in),int,own))
{
	string_handle* h = static_cast<string_handle*>(s);
	if (h && h->m_ptr && h->m_vtbl && h->m_vtbl->addref)
		(*h->m_vtbl->addref)(h,own);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_release,1,((in),void*,s))
{
	string_handle* h = static_cast<string_handle*>(s);
	if (h)
	{
		if (h->m_ptr && h->m_vtbl && h->m_vtbl->release)
			(*h->m_vtbl->release)(h->m_ptr);

		h->m_ptr = NULL;
		h->m_vtbl = NULL;
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_assign,2,((in),void*,s1,(in),const void*,s2))
{
	string_handle h = *static_cast<const string_handle*>(s2);
	OOCore_string_t_addref(&h,0);
	OOCore_string_t_release(s1);
	*static_cast<string_handle*>(s1) = h;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const char*,OOCore_string_t_cast,2,((in),const void*,s,(in),size_t*,plen))
{
	const string_handle* h = static_cast<const string_handle*>(s);
	if (h && h->m_ptr && h->m_vtbl)
	{
		if (plen)
			*plen = (*h->m_vtbl->length)(h->m_ptr);

		return (*h->m_vtbl->buffer)(h->m_ptr);
	}

	if (plen)
		*plen = 0;

	return "";
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_append1,2,((in),void*,s1,(in),const void*,s2))
{
	if (s1)
	{
		size_t add_len = 0;
		const char* add_buffer = OOCore_string_t_cast(s2,&add_len);
		if (add_len > 0)
		{
			int err = 0;
			char* buffer = string_grow(s1,add_len,err);
			if (err != 0)
				OMEGA_THROW(err);

			if (buffer)
				memcpy(buffer,add_buffer,add_len);
			else
			{
				size_t orig_len = 0;
				const char* orig_buffer = OOCore_string_t_cast(s1,&orig_len);

				StringNode* pNode = new (OOCore::throwing) StringNode();
				buffer = pNode->grow(orig_len + add_len,err);
				if (!buffer || err != 0)
				{
					delete pNode;
					OMEGA_THROW(err ? err : ERROR_OUTOFMEMORY);
				}

				memcpy(buffer,orig_buffer,orig_len);
				memcpy(buffer+orig_len,add_buffer,add_len);

				OOCore_string_t_release(s1);

				string_handle* h = static_cast<string_handle*>(s1);
				h->m_vtbl = &s_string_vtbl;
				h->m_ptr = pNode;
			}
		}
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_append2,3,((in),void*,s,(in),const char*,sz,(in),size_t,len))
{
	if (len == string_t::npos)
		len = strlen(sz);

	if (len > 0)
	{
		if (!s)
			OOCore_string_t__ctor(s,sz,len);
		else
		{
			int err = 0;
			char* buffer = string_grow(s,len,err);
			if (err != 0)
				OMEGA_THROW(err);

			if (buffer)
				memcpy(buffer,sz,len);
			else
			{
				size_t orig_len = 0;
				const char* orig_buffer = OOCore_string_t_cast(s,&orig_len);

				StringNode* pNode = new (OOCore::throwing) StringNode();
				buffer = pNode->grow(orig_len + len,err);
				if (!buffer || err != 0)
				{
					delete pNode;
					OMEGA_THROW(err ? err : ERROR_OUTOFMEMORY);
				}

				memcpy(buffer,orig_buffer,orig_len);
				memcpy(buffer+orig_len,sz,len);

				OOCore_string_t_release(s);

				string_handle* h = static_cast<string_handle*>(s);
				h->m_vtbl = &s_string_vtbl;
				h->m_ptr = pNode;
			}
		}
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cmp1,4,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos,(in),size_t,length))
{
	size_t len1 = 0;
	const char* buf1 = OOCore_string_t_cast(s1,&len1);

	size_t len2 = 0;
	const char* buf2 = OOCore_string_t_cast(s2,&len2);

	return compare(buf1,len1,buf2,len2,pos,length);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_string_t_cmp2,2,((in),const void*,s,(in),const char*,sz))
{
	size_t len1 = 0;
	const char* buf = OOCore_string_t_cast(s,&len1);

	size_t len2 = 0;
	if (sz)
		len2 = strlen(sz);

	return compare(buf,len1,sz,len2,0,len1);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find1,3,((in),const void*,s,(in),char,c,(in),size_t,pos))
{
	size_t len = 0;
	const char* start = OOCore_string_t_cast(s,&len);
	if (pos >= len)
		len = 0;
	else
		len -= pos;

	if (len == 0)
		return string_t::npos;

	const char* found = static_cast<const char*>(memchr(start + pos,c,len));
	if (!found)
		return string_t::npos;

	return static_cast<size_t>(found - start);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find2,3,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos))
{
	size_t len = 0;
	const char* start = OOCore_string_t_cast(s1,&len) + pos;
	if (pos >= len)
		len = 0;
	else
		len -= pos;

	size_t search_len = 0;
	const char* search = OOCore_string_t_cast(s2,&search_len);

	if (search_len > len)
		return string_t::npos;

	if (search_len == 0)
		return 0;

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

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_not,3,((in),const void*,s,(in),char,c,(in),size_t,pos))
{
	size_t len = 0;
	const char* start = OOCore_string_t_cast(s,&len) + pos;
	
	for (size_t i=pos;i < len;++i)
	{
		if (*(start++) != c)
			return i;
	}
	
	return string_t::npos;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_oneof,3,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos))
{
	size_t len = 0;
	const char* start = OOCore_string_t_cast(s1,&len) + pos;
	if (pos >= len)
		len = 0;
	else
		len -= pos;

	if (len == 0)
		return string_t::npos;

	size_t search_len = 0;
	const char* search = OOCore_string_t_cast(s2,&search_len);

	if (search_len > 0)
	{
		for (size_t offset = pos;len != 0;start += offset + 1)
		{
			const char* found = strpbrk(start,search);
			if (found)
				return offset + static_cast<size_t>(found - start);

			size_t p = strlen(start);
			offset += p;
			len -= p;
		}
	}

	return string_t::npos;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_find_notof,3,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos))
{
	size_t len = 0;
	const char* start = OOCore_string_t_cast(s1,&len) + pos;

	if (pos >= len)
		len = 0;
	else
		len -= pos;

	if (len == 0)
		return string_t::npos;

	size_t search_len = 0;
	const char* search = OOCore_string_t_cast(s2,&search_len);

	if (search_len > 0)
	{
		for (size_t offset = pos;len != 0;start += offset + 1)
		{
			size_t p = strlen(start);
			size_t f = strspn(start,search);
			if (f < p)
				return offset + f;

			offset += p;
			len -= p;
		}
	}

	return string_t::npos;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_string_t_rfind,3,((in),const void*,s,(in),char,c,(in),size_t,pos))
{
	size_t len = 0;
	const char* start = OOCore_string_t_cast(s,&len);

	if (pos >= len)
		pos = len-1;
	
	if (len > 0)
	{
		start += pos;
		for (size_t i=pos+1;i > 0;--i)
		{
			if (*(start--) == c)
				return i-1;
		}
	}

	return string_t::npos;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_left,4,((in),void*,s1,(in),const void*,s2,(in),size_t,start,(in),size_t,length))
{
	size_t orig_len = 0;
	const char* buffer = OOCore_string_t_cast(s2,&orig_len);

	if (start >= orig_len)
		OOCore_string_t__ctor(s1,NULL,0);
	else
	{
		if (length > orig_len - start)
			length = orig_len - start;

		if (start == 0 && orig_len == length)
		{
			*static_cast<string_handle*>(s1) = *static_cast<const string_handle*>(s2);
			OOCore_string_t_addref(s1,0);
		}
		else
		{
			OOCore_string_t__ctor(s1,buffer+start,length);
		}
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_string_t_right,3,((in),void*,s1,(in),const void*,s2,(in),size_t,length))
{
	size_t orig_len = 0;
	const char* buffer = OOCore_string_t_cast(s2,&orig_len);

	if (orig_len <= length)
	{
		*static_cast<string_handle*>(s1) = *static_cast<const string_handle*>(s2);
		OOCore_string_t_addref(s1,0);
	}
	else
	{
		OOCore_string_t__ctor(s1,buffer+(orig_len - length),length);
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
		(uint32_t)guid.Data1,guid.Data2,guid.Data3,
		guid.Data4[0],guid.Data4[1],guid.Data4[2],guid.Data4[3],
		guid.Data4[4],guid.Data4[5],guid.Data4[6],guid.Data4[7]);

	if (err != 0)
		OMEGA_THROW(err);

	return str.c_str();
}

namespace
{
	int guid_t_from_string(const char* sz, guid_base_t* result)
	{
		// Do this manually...
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
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_guid_t_from_string,2,((in),const char*,sz,(in_out),guid_base_t*,result))
{
	int ret = guid_t_from_string(sz,result);
	if (ret == 0)
	{
		result->Data1 = 0;
		result->Data2 = 0;
		result->Data3 = 0;
		memset(result->Data4,sizeof(result->Data4),0);
	}
	return ret;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(guid_t,OOCore_guid_t_create,0,())
{
#if defined(_WIN32)

	UUID uuid = {0};
	if (UuidCreate(&uuid) != RPC_S_OK)
		OMEGA_THROW(GetLastError());

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

	guid_base_t res;
	int err = OOBase::POSIX::random_bytes(&res,sizeof(res));
	if (err)
		OMEGA_THROW(err);

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
