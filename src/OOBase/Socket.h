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

#ifndef OOBASE_SOCKET_H_INCLUDED_
#define OOBASE_SOCKET_H_INCLUDED_

#include "TimeVal.h"
#include "Buffer.h"

namespace OOBase
{
	class Socket
	{
	public:
		virtual ~Socket() {}

		virtual int send(const void* buf, size_t len, const timeval_t* timeout = 0) = 0;

		template <class T>
		int send(const T& val, const timeval_t* timeout = 0)
		{
			return send(&val,sizeof(T),timeout);
		}

		int send_buffer(const Buffer* buffer, const timeval_t* timeout = 0)
		{
			return send(buffer->rd_ptr(),buffer->length(),timeout);
		}

		virtual size_t recv(void* buf, size_t len, int* perr, const timeval_t* timeout = 0) = 0;

		template <class T>
		int recv(T& val, const timeval_t* timeout = 0)
		{
			int err = 0;
			recv(&val,sizeof(T),&err,timeout);
			return err;
		}

		int recv_buffer(Buffer* buffer, size_t len, const timeval_t* timeout = 0)
		{
			int err = buffer->space(len);
			if (err == 0)
			{
				len = recv(buffer->wr_ptr(),len,&err,timeout);
				if (err == 0)
					buffer->wr_ptr(len);
			}
			return err;
		}

		virtual void close() = 0;

	protected:
		Socket() {}
		Socket(const Socket&) {}
		Socket& operator = (const Socket&) { return *this; }
	};

	class LocalSocket : public Socket
	{
	public:
#if defined(_WIN32)
		typedef HANDLE uid_t;
#elif defined(HAVE_UNISTD_H)
		typedef ::uid_t uid_t;
#else
#error Fix me!
#endif
		virtual uid_t get_uid() = 0;

		static LocalSocket* connect_local(const std::string& path, int* perr, const timeval_t* wait = 0);

	protected:
		LocalSocket() {}
		LocalSocket(const LocalSocket&) {}
		LocalSocket& operator = (const LocalSocket&) { return *this; }
	};
}

#endif // OOBASE_SOCKET_H_INCLUDED_
