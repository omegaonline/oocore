///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOSvrBase, the Omega Online Base library.
//
// OOSvrBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOSvrBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOSVRBASE_PW_INFO_H_INCLUDED_
#define OOSVRBASE_PW_INFO_H_INCLUDED_

#include "SmartPtr.h"

#if defined(HAVE_PWD_H)

#include <pwd.h>

namespace OOSvrBase
{
	class pw_info
	{
	public:
		pw_info(uid_t uid);
		pw_info(const char* uname);

		inline struct passwd* operator ->()
		{
			return m_pwd;
		}

		inline bool operator !() const
		{
			return (m_pwd==0);
		}

	private:
		pw_info() {};

		struct passwd* m_pwd;
		struct passwd  m_pwd2;
		size_t         m_buf_len;

		OOBase::SmartPtr<char,OOBase::ArrayDestructor<char> > m_buffer;
	};
}

#endif

#endif // OOSVRBASE_PW_INFO_H_INCLUDED_
