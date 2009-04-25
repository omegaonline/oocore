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

#ifndef OOBASE_BUFFER_H_INCLUDED_
#define OOBASE_BUFFER_H_INCLUDED_

#include "Atomic.h"

namespace OOBase
{
	/// Used to read and write data to a variable length memory buffer
	/**
	 *	A Buffer instance maintains two pointers, one to the read position: rd_ptr(),
	 *	and one to the write position: wr_ptr(). 
	 *	
	 *	The size of the internal buffer maintained by the class is controlled by space(), 
	 *	but care should be exercised because the values returned by rd_ptr() and wr_ptr() 
	 *	might change due to reallocation.
	 *
	 *	The following diagram shows the relationships between the read and write pointers 
	 *	and the internal buffer:
	 *
	 *	\verbatim
		  *.............*...............*..............*
		m_buffer     rd_ptr()        wr_ptr()     m_capacity
						|<------------->|<------------>|
							 length()        space()      \endverbatim
	 *	\warning This class is not thread-safe.
	 */
	class Buffer
	{
	public:
		/// The constructor allocates the internal buffer to size \p cbSize.
		Buffer(size_t cbSize = 256);
		
		/// Return a reference counted copy
		Buffer* duplicate();

		/// Release a reference
		void release();
		
		/// Get the current read pointer value.
		const char* rd_ptr() const;

		/// Advance the read pointer by \p cbSkip bytes.
		void rd_ptr(size_t cbSkip);

		/// Advance the read pointer to \p align byte boundary.
		void align_rd_ptr(size_t align);
		
		/// Get the current write pointer value.
		char* wr_ptr();

		/// Advance the write pointer by \p cbExpand bytes.
		int wr_ptr(size_t cbExpand);

		/// Advance the write pointer to \p align byte boundary.
		int align_wr_ptr(size_t align);
		
		/// Get the used length of the buffer, the difference between rd_ptr() and wr_ptr().
		size_t length() const;
		
		/// Reset the read and write pointers to start().
		int reset(size_t align = 1);

		/// Get the amount of space remaining in bytes.
		size_t space() const;

		/// Adjust the amount of space remaining.
		int space(size_t cbSpace);

		/// Return rd_ptr as an offset
		size_t mark_rd_ptr() const;

		/// Move rd_ptr to mark
		void mark_rd_ptr(size_t mark);

		/// Return wr_ptr as an offset
		size_t mark_wr_ptr() const;

		/// Move wr_ptr to mark
		void mark_wr_ptr(size_t mark);

	private:
		Buffer(const Buffer& rhs);
		Buffer& operator = (const Buffer& rhs);

		~Buffer();

		AtomicInt<unsigned long> m_refcount; ///< The reference count.

		size_t	m_capacity;	///< The total allocated bytes for \p m_buffer.
		char*	m_buffer;	///< The actual underlying buffer.
		char*	m_wr_ptr;	///< The current write pointer.
		char*	m_rd_ptr;	///< The current read pointer.

		int priv_malloc(char*& ptr, size_t& bytes);
		int priv_realloc(char*& ptr, size_t& bytes);
		void priv_free(char* ptr);
	};
}

#endif // OOBASE_BUFFER_H_INCLUDED_
