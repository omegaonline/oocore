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

#ifndef OOBASE_WIN32_SOCKET_H_INCLUDED_
#define OOBASE_WIN32_SOCKET_H_INCLUDED_

#include "Socket.h"

#if defined(_WIN32)

namespace OOBase
{
	namespace Win32
	{
		class SocketImpl
		{
		public:
			SocketImpl(HANDLE hSocket);
			virtual ~SocketImpl();

			virtual int send(const void* buf, size_t len, const OOBase::timeval_t* timeout = 0);
			virtual size_t recv(void* buf, size_t len, int* perr, const OOBase::timeval_t* timeout = 0);
			virtual void close();

			HANDLE peek_handle() const
			{
				return m_hSocket;
			}
		
		protected:
			SmartHandle m_hSocket;

		private:
			HANDLE m_hReadEvent;
			HANDLE m_hWriteEvent;
		};

		template <typename Base>
		class SocketTempl : 
			public Base,
			public SocketImpl
		{
		public:
			SocketTempl(HANDLE hSocket) :
				SocketImpl(hSocket)
			{}

			virtual ~SocketTempl()
			{}

			int send(const void* buf, size_t len, const timeval_t* timeout = 0)
			{
				return SocketImpl::send(buf,len,timeout);
			}

			size_t recv(void* buf, size_t len, int* perr, const timeval_t* timeout = 0)
			{
				return SocketImpl::recv(buf,len,perr,timeout);
			}

			void close()
			{
				SocketImpl::close();
			}

		private:
			SocketTempl(const SocketTempl&);
			SocketTempl& operator = (const SocketTempl&);
		};

		class Socket : 
			public SocketTempl<OOBase::Socket>
		{
		public:
			Socket(HANDLE hSocket) :
				SocketTempl<OOBase::Socket>(hSocket)
			{}
		};

		class LocalSocket : 
			public SocketTempl<OOBase::LocalSocket>
		{
		public:
			LocalSocket(HANDLE hSocket) :
				SocketTempl<OOBase::LocalSocket>(hSocket)
			{}

			virtual OOBase::LocalSocket::uid_t get_uid();
		};
	}
}

#endif // defined(_WIN32)

#endif // OOBASE_WIN32_SOCKET_H_INCLUDED_
