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

#ifndef OOHTTP_QUICK_BUFFER_H_INCLUDED_
#define OOHTTP_QUICK_BUFFER_H_INCLUDED_

namespace OOHttp
{
	class QuickBuffer
	{
	public:
		QuickBuffer();
		~QuickBuffer();

		void append(const char* bytes, size_t lenBytes);
		void clear();
		void compact();
		void cache();
		
		const char* begin() const;
		const char* end() const;
		size_t length() const;

		void tell(size_t bytes);

	private:
		const char*  m_ptr;
		
		char*       m_cached;
		char*       m_cached_end;
		size_t      m_cached_len;
		const char* m_extern;
		size_t      m_extern_len;

		void grow(size_t len);
	};
}

#endif // OOHTTP_QUICK_BUFFER_H_INCLUDED_
