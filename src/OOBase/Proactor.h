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

#if !defined(_WIN32)
typedef struct
{
	mode_t mode;
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
		AsyncSocket(const AsyncSocket&);
		AsyncSocket& operator = (const AsyncSocket&);

		OOBase::AtomicInt<size_t> m_ref_count;
	};

	class IOHandler
	{
	public:
		virtual void on_read(AsyncSocket* /*pSocket*/, OOBase::Buffer* /*buffer*/, int /*err*/) {}
		virtual void on_write(AsyncSocket* /*pSocket*/, OOBase::Buffer* /*buffer*/, int /*err*/) {}
		virtual void on_closed(AsyncSocket* /*pSocket*/) {}

	protected:
		virtual ~IOHandler() {}
	};

	class Acceptor
	{
	public:
		virtual bool on_accept(OOBase::Socket* pSocket, int err) = 0;

	protected:
		virtual ~Acceptor() {}
	};

	namespace detail
	{
		class ProactorImpl
		{
		public:
			ProactorImpl() {}
			virtual ~ProactorImpl() {}

			virtual OOBase::Socket* accept_local(Acceptor* handler, const std::string& path, int* perr, SECURITY_ATTRIBUTES* psa) = 0;
			virtual AsyncSocket* attach_socket(IOHandler* handler, int* perr, OOBase::Socket* sock) = 0;

		private:
			ProactorImpl(const ProactorImpl&);
			ProactorImpl& operator = (const ProactorImpl&);
		};
	}

	class Proactor
	{
	public:
		Proactor();
		~Proactor();

		OOBase::Socket* accept_local(Acceptor* handler, const std::string& path, int* perr, SECURITY_ATTRIBUTES* psa = 0);

		AsyncSocket* attach_socket(IOHandler* handler, int* perr, OOBase::Socket* sock);

	private:
		Proactor(const Proactor&);
		Proactor& operator = (const Proactor&);

		detail::ProactorImpl* m_impl;
	};
}

#endif // OOSVRBASE_PROACTOR_H_INCLUDED_
