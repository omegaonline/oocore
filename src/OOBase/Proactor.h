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

#ifndef OOSVRBASE_PROACTOR_H_INCLUDED_
#define OOSVRBASE_PROACTOR_H_INCLUDED_

#include "TimeVal.h"
#include "Buffer.h"
#include "Socket.h"

#include <string>

#if !defined(_WIN32)
typedef struct
{
	int unused;
} SECURITY_ATTRIBUTES;
#endif

namespace OOSvrBase
{
	class Proactor;
		
	class AsyncSocket
	{
	public:
		virtual int read(OOBase::Buffer* buffer, size_t len = 0) = 0;
		virtual int write(OOBase::Buffer* buffer) = 0;
		virtual void close() = 0;

		void addref()
		{
			++m_ref_count;
		}

		void release()
		{
			if (--m_ref_count == 0)
				delete this;
		}

	protected:
		AsyncSocket() : m_ref_count(1)
		{}

		virtual ~AsyncSocket() {}

	private:
		AsyncSocket(const AsyncSocket&) {}
		AsyncSocket& operator = (const AsyncSocket&) { return *this; }

		OOBase::AtomicInt<size_t> m_ref_count;
	};

	class IOHandler
	{
	public:
		virtual void on_read(AsyncSocket* /*pSocket*/, OOBase::Buffer* /*buffer*/, int /*err*/) {}
		virtual void on_write(AsyncSocket* /*pSocket*/, OOBase::Buffer* /*buffer*/, int /*err*/) {}
		virtual void on_closed(AsyncSocket* /*pSocket*/) {}
	};

	class Acceptor
	{
	public:
		virtual bool on_accept(OOBase::Socket* pSocket, int err) = 0;
	};

	class AsyncAcceptor
	{
	public:
		virtual IOHandler* make_handler() = 0;
		virtual bool on_accept(IOHandler* handler, AsyncSocket* pSocket, int err) = 0;
	};

	class ProactorImpl;

	class Proactor
	{
	public:
		Proactor();
		~Proactor();

		OOBase::Socket* accept_local(Acceptor* handler, const std::string& path, int* perr, SECURITY_ATTRIBUTES* psa = 0);

		AsyncSocket* accept_shared_mem_socket(const std::string& strName, IOHandler* handler, int* perr, OOBase::LocalSocket* via, OOBase::timeval_t* timeout = 0, SECURITY_ATTRIBUTES* psa = 0);
		AsyncSocket* connect_shared_mem_socket(IOHandler* handler, int* perr, OOBase::LocalSocket* via, OOBase::timeval_t* timeout = 0);

	private:
		Proactor(const Proactor&) {}
		Proactor& operator = (const Proactor&) { return *this; }

		ProactorImpl* m_impl;
	};
}

#endif // OOSVRBASE_PROACTOR_H_INCLUDED_