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

#ifndef OOBASE_POSIX_SOCKET_H_INCLUDED_
#define OOBASE_POSIX_SOCKET_H_INCLUDED_

#include "Socket.h"

#if !defined(_WIN32) && defined(HAVE_SYS_SOCKET_H)

#include <sys/socket.h>

namespace OOBase
{
	namespace POSIX
	{
		class SocketImpl
		{
		public:
			SocketImpl(int fd);
			virtual ~SocketImpl();

			virtual int send(const void* buf, size_t len, const OOBase::timeval_t* timeout = 0);
			virtual size_t recv(void* buf, size_t len, int* perr, const OOBase::timeval_t* timeout = 0);
			virtual void close();

			int get_fd() const
			{
				return m_fd;
			}

		protected:
			int m_fd;
		};

		template <class Base>
		class SocketTempl :
			public Base,
			public SocketImpl
		{
		public:
			SocketTempl(int fd) :
				SocketImpl(fd)
			{}

			virtual ~SocketTempl()
			{}

			int send(const void* buf, size_t len, const OOBase::timeval_t* timeout = 0)
			{
				return SocketImpl::send(buf,len,timeout);
			}

			size_t recv(void* buf, size_t len, int* perr, const OOBase::timeval_t* timeout = 0)
			{
				return SocketImpl::recv(buf,len,perr,timeout);
			}

			void close()
			{
				SocketImpl::close();
			}

		private:
			SocketTempl(const SocketTempl&) {}
			SocketTempl& operator = (const SocketTempl&) { return *this; }
		};

		class Socket :
			public SocketTempl<OOBase::Socket>
		{
		public:
			Socket(int fd) :
				SocketTempl<OOBase::Socket>(fd)
			{}
		};

		class LocalSocket :
			public SocketTempl<OOBase::LocalSocket>
		{
		public:
			LocalSocket(int fd, const std::string& path) :
				SocketTempl<OOBase::LocalSocket>(fd),
				m_path(path)
			{}

			virtual ~LocalSocket()
			{
			#if defined(HAVE_UNISTD_H)
				if (!m_path.empty())
					unlink(m_path.c_str());
			#endif
			}

			virtual OOBase::LocalSocket::uid_t get_uid();

		private:
			std::string m_path;
		};
	}
}

#endif // !defined(_WIN32) && defined(HAVE_SYS_SOCKET_H)

#endif // OOBASE_POSIX_SOCKET_H_INCLUDED_
