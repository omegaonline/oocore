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

#ifndef OOSVRBASE_PROACTOR_WIN32_H_INCLUDED_
#define OOSVRBASE_PROACTOR_WIN32_H_INCLUDED_

#if !defined(_WIN32)
#error Includes have got confused, check Proactor.h
#endif

namespace OOSvrBase
{
	class HandleSocket : public OOSvrBase::AsyncSocket
	{
	public:
		HandleSocket(ProactorImpl* pProactor, HANDLE handle);
		int init(OOSvrBase::IOHandler* handler);
				
		int read(OOBase::Buffer* buffer, size_t len);
		int write(OOBase::Buffer* buffer);
		void close();

	private:
		virtual ~HandleSocket();

		static VOID CALLBACK completion_fn(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

		struct Completion
		{
			OVERLAPPED      ov;
			HandleSocket*   this_ptr;
			OOBase::Buffer* buffer;
			bool            is_reading;
			size_t          to_read;
		};

		DWORD do_read(Completion* pInfo, DWORD dwToRead);
		void handle_read(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, Completion* pInfo);

		DWORD do_write(Completion* pInfo);
		void handle_write(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, Completion* pInfo);

		ProactorImpl*              m_pProactor;
		OOBase::Win32::SmartHandle m_handle;
		OOSvrBase::IOHandler*      m_handler;
	};

	class ProactorImpl
	{
	public:
		ProactorImpl();
		~ProactorImpl();

		OOBase::Socket* accept_local(Acceptor* handler, const std::string& path, int* perr, SECURITY_ATTRIBUTES* psa);
		
		AsyncSocket* attach_socket(IOHandler* handler, int* perr, OOBase::Socket* sock);

		OOBase::AtomicInt<size_t> m_outstanding;

	private:
		ProactorImpl(const ProactorImpl&) {}
		ProactorImpl& operator = (const ProactorImpl&) { return *this; }
	};
}

#endif // OOSVRBASE_PROACTOR_WIN32_H_INCLUDED_
