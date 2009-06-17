///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOBase, the Omega Online Base library.
//
// OOBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "Buffer.h"

OOBase::Buffer::Buffer(size_t cbSize) :
	m_refcount(1),
	m_capacity(0),
	m_buffer(0),
	m_wr_ptr(0),
	m_rd_ptr(0)
{
	int err = priv_malloc(m_buffer,cbSize);
	if (err == 0)
	{
		m_capacity = cbSize;
		reset();
	}
}

OOBase::Buffer::Buffer(const Buffer& rhs) :
	m_refcount(1),
	m_capacity(0),
	m_buffer(0),
	m_wr_ptr(0),
	m_rd_ptr(0)
{
	size_t cbSize = rhs.m_capacity;
	int err = priv_malloc(m_buffer,cbSize);
	if (err == 0)
	{
		m_capacity = cbSize;
		memcpy(m_buffer,rhs.m_buffer,rhs.m_capacity);

		m_rd_ptr = m_buffer + (rhs.m_rd_ptr - rhs.m_buffer);
		m_wr_ptr = m_buffer + (rhs.m_wr_ptr - rhs.m_buffer);
	}
}

OOBase::Buffer& OOBase::Buffer::operator = (const Buffer& rhs)
{
	if (&rhs != this)
	{
		size_t cbSize = rhs.m_capacity;
		int err = priv_realloc(m_buffer,cbSize);
		if (err == 0)
		{
			m_capacity = cbSize;
			memcpy(m_buffer,rhs.m_buffer,rhs.m_capacity);

			m_rd_ptr = m_buffer + (rhs.m_rd_ptr - rhs.m_buffer);
			m_wr_ptr = m_buffer + (rhs.m_wr_ptr - rhs.m_buffer);
		}
	}

	return *this;
}

OOBase::Buffer* OOBase::Buffer::duplicate()
{
	++m_refcount;
	return this;
}

void OOBase::Buffer::release()
{
	if (--m_refcount == 0)
		delete this;
}

OOBase::Buffer::~Buffer()
{
	free(m_buffer);
}

const char* OOBase::Buffer::rd_ptr() const
{
	return m_rd_ptr;
}

size_t OOBase::Buffer::mark_rd_ptr() const
{
	return static_cast<size_t>(m_rd_ptr - m_buffer);
}

void OOBase::Buffer::mark_rd_ptr(size_t mark)
{
	m_rd_ptr = (m_buffer + mark);
}

void OOBase::Buffer::rd_ptr(size_t cbSkip)
{
	m_rd_ptr += cbSkip;

	if (m_rd_ptr > m_wr_ptr)
		m_rd_ptr = m_wr_ptr;
}

void OOBase::Buffer::align_rd_ptr(size_t align)
{
	size_t overrun = (reinterpret_cast<uintptr_t>(m_rd_ptr) & (align-1));
	if (overrun)
		rd_ptr(align - overrun);
}

char* OOBase::Buffer::wr_ptr()
{
	return m_wr_ptr;
}

size_t OOBase::Buffer::mark_wr_ptr() const
{
	return static_cast<size_t>(m_wr_ptr - m_buffer);
}

void OOBase::Buffer::mark_wr_ptr(size_t mark)
{
	m_wr_ptr = (m_buffer + mark);
}

int OOBase::Buffer::wr_ptr(size_t cbExpand)
{
	int err = space(cbExpand);
	if (err == 0)
		m_wr_ptr += cbExpand;
	return err;
}

int OOBase::Buffer::align_wr_ptr(size_t align)
{
	int err = 0;
	size_t overrun = (reinterpret_cast<uintptr_t>(m_wr_ptr) & (align-1));
	if (overrun)
	{
		overrun = align - overrun;
		err = space(overrun);
		if (err == 0)
		{
			memset(m_wr_ptr,0xee,overrun);
			wr_ptr(overrun);
		}
	}
	return err;
}

size_t OOBase::Buffer::length() const
{
	return static_cast<size_t>(m_wr_ptr - m_rd_ptr);
}

int OOBase::Buffer::reset(size_t align)
{
	int err = 0;
	m_rd_ptr = m_wr_ptr = m_buffer;

	if (align > 1)
	{
		err = align_wr_ptr(align);
		if (err == 0)
			align_rd_ptr(align);
	}
	return err;
}

size_t OOBase::Buffer::space() const
{
	size_t used = static_cast<size_t>(m_wr_ptr - m_buffer);

	return (used >= m_capacity ? 0 : m_capacity - used);
}

/**
 *	\warning A reallocation may occur updating rd_ptr() and wr_ptr().
 */
int OOBase::Buffer::space(size_t cbSpace)
{
	int err = 0;
	size_t cbAbsCapacity = (m_wr_ptr - m_buffer) + cbSpace;
	if (cbAbsCapacity > m_capacity)
	{
		size_t rd_pos = (m_rd_ptr - m_buffer);
		size_t wr_pos = (m_wr_ptr - m_buffer);

		err = priv_realloc(m_buffer,cbAbsCapacity);
		if (err == 0)
		{
			m_rd_ptr = m_buffer + rd_pos;
			m_wr_ptr = m_buffer + wr_pos;
			m_capacity = cbAbsCapacity;
		}
	}
	return err;
}

//#if defined(_WIN32)

// Possible better implementation?

//#else

int OOBase::Buffer::priv_malloc(char*& ptr, size_t& bytes)
{
	ptr = (char*)malloc(bytes);
	return (!ptr ? errno : 0);
}

int OOBase::Buffer::priv_realloc(char*& ptr, size_t& bytes)
{
	char* new_ptr = (char*)realloc(ptr,bytes);
	if (!new_ptr)
		return errno;

	ptr = new_ptr;
	return 0;
}

void OOBase::Buffer::priv_free(char* ptr)
{
	free(ptr);
}

//#endif
