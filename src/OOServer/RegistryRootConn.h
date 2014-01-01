///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2013 Rick Taylor
//
// This file is part of OOSvrReg, the Omega Online registry server.
//
// OOSvrReg is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrReg is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOSvrReg.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOSERVER_REGISTRY_ROOT_CONN_H_INCLUDED_
#define OOSERVER_REGISTRY_ROOT_CONN_H_INCLUDED_

namespace Registry
{
	class Manager;

	class RootConnection : public OOBase::RefCounted
	{
	public:
		RootConnection(Manager* pManager, OOBase::RefPtr<OOBase::AsyncSocket>& sock);

		bool start();

	private:
		virtual ~RootConnection();

		Manager*                            m_pManager;
		OOBase::RefPtr<OOBase::AsyncSocket> m_socket;

		OOBase::Proactor* get_proactor();
#if defined(_WIN32)
		class PipeConnection :
				public OOBase::RefCounted,
				public OOBase::AllocatorNew<OOBase::CrtAllocator>
		{
		public:
			PipeConnection(RootConnection* parent) : m_parent(parent)
			{}

			int start(DWORD pid, OOBase::LocalString& strPipe, const char* pszSID);

		private:
			RootConnection*                  m_parent;
			OOBase::RefPtr<OOBase::Acceptor> m_wait;
			OOBase::RefPtr<OOBase::Acceptor> m_pipe;

			static void on_wait(void* param, HANDLE hObject, bool bTimedout, int err);
			static void on_accept(void* param, OOBase::AsyncSocket* pSocket, int err);
		};
		friend class PipeConnection;

		OOBase::SpinLock m_lock;
		OOBase::HashTable<const PipeConnection*,OOBase::RefPtr<PipeConnection> > m_mapConns;

		void on_message_win32(OOBase::CDRStream& stream, int err);
		void on_message(OOBase::CDRStream& stream);
		void new_connection(OOBase::CDRStream& stream);
#elif defined(HAVE_UNISTD_H)
		void on_message_posix(OOBase::CDRStream& stream, OOBase::Buffer* ctl_buffer, int err);
		void on_message(OOBase::CDRStream& stream, OOBase::POSIX::SmartFD& passed_fd);
		void new_connection(OOBase::CDRStream& stream, OOBase::POSIX::SmartFD& passed_fd);
#else
#error Implement platform native credential and pipe handle passing
#endif

		void on_sent(OOBase::Buffer* buffer, int err);
		bool recv_next();
	};
}

#endif // OOSERVER_REGISTRY_ROOT_CONN_H_INCLUDED_
