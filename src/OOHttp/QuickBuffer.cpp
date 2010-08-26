///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
//
// This file is part of OOHttpd, the Omega Online HTTP Server application.
//
// OOHttpd is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOHttpd is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOHttpd.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOHttpd.h"
#include "QuickBuffer.h"

OOHttp::QuickBuffer::QuickBuffer() :
		m_ptr(0),
		m_cached(0),
		m_cached_end(0),
		m_cached_len(0),
		m_extern(0),
		m_extern_len(0)
{
}

OOHttp::QuickBuffer::~QuickBuffer()
{
	if (m_cached)
		free(m_cached);
}

void OOHttp::QuickBuffer::append(const char* bytes, size_t lenBytes)
{
	// If we have something cached, then append to the cache,
	// else use the extern
	if (m_cached_end > m_cached)
	{
		ptrdiff_t off = (m_ptr - m_cached);
		
		grow(lenBytes);

		memcpy(m_cached_end,bytes,lenBytes);
		m_cached_end += lenBytes;
		m_ptr = m_cached + off;

		m_extern = 0;
		m_extern_len = 0;
	}
	else
	{
		m_ptr = m_extern = bytes;
		m_extern_len = lenBytes;
	}
}

void OOHttp::QuickBuffer::clear()
{
	m_ptr = m_cached_end = m_cached;
	m_extern = 0;
	m_extern_len = 0;
}

void OOHttp::QuickBuffer::cache()
{
	// Only cache if we are not using the cache
	if (m_cached_end == m_cached)
	{
		size_t len = length();
		if (len)
		{
			grow(len);

			memcpy(m_cached,m_ptr,len);

			m_ptr = m_cached;
			m_cached_end = m_cached + len;
		}
	}
}

void OOHttp::QuickBuffer::grow(size_t len)
{
	char* n = (char*)realloc(m_cached,len + m_cached_len);
	if (!n)
		OMEGA_THROW(ENOMEM);

	if (n != m_cached)
	{
		m_cached_end = n + (m_cached_end - m_cached);
		m_cached = n;
	}

	m_cached_len += len;
}

void OOHttp::QuickBuffer::compact()
{
	// We only compact cached data
	if (m_cached_end > m_cached)
	{
		if (m_ptr < m_cached_end)
		{
			size_t len = size_t(m_cached_end - m_ptr);

			memmove(m_cached,m_ptr,len);

			m_cached_end = m_cached + len;
			m_ptr = m_cached;
		}
		else
		{
			m_ptr = m_cached_end = m_cached;
		}
	}
}

const char* OOHttp::QuickBuffer::begin() const
{
	return m_ptr;
}

const char* OOHttp::QuickBuffer::end() const
{
	if (m_cached_end > m_cached)
		return m_cached_end;
	else
		return m_ptr + m_extern_len;
}

size_t OOHttp::QuickBuffer::length() const
{
	if (m_cached_end > m_cached)
		return (m_cached_end - m_ptr);
	else
		return m_extern_len - (m_ptr - m_extern);
}

void OOHttp::QuickBuffer::tell(size_t bytes)
{
	m_ptr += bytes;

	const char* end_ptr = end();
	if (m_ptr > end_ptr)
		m_ptr = end_ptr;
}
