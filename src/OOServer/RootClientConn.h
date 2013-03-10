///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2013 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
//
//  ***** THIS IS A SECURE MODULE *****
//
//  It will be run as Administrator/setuid root
//
//  Therefore it needs to be SAFE AS HOUSES!
//
//  Do not include anything unnecessary
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_ROOT_CLIENT_CONN_H_INCLUDED_
#define OOSERVER_ROOT_CLIENT_CONN_H_INCLUDED_

namespace Root
{
	class ClientConnection : public OOBase::RefCounted
	{
	public:
		ClientConnection(Manager* pManager, OOBase::RefPtr<OOBase::AsyncSocket>& sock);

		bool start();

		pid_t get_pid() const;
		const uid_t& get_uid() const;
		const char* get_session_id() const;

#if defined(HAVE_UNISTD_H)
		bool send_response(OOBase::POSIX::SmartFD& fd, pid_t pid);
#endif

	private:
		Manager*                            m_pManager;
		OOBase::RefPtr<OOBase::AsyncSocket> m_socket;

		pid_t          m_pid;
		uid_t          m_uid;
		OOBase::String m_session_id;

#if defined(_WIN32)
		void on_message_win32(OOBase::CDRStream& stream, int err);
#elif defined(HAVE_UNISTD_H)
		void on_message_posix(OOBase::CDRStream& stream, OOBase::Buffer* ctl_buffer, int err);
#endif

		void on_message(OOBase::CDRStream& stream, int err);
		void on_done(OOBase::Buffer* data_buffer, OOBase::Buffer* ctl_buffer, int err);

	public:
		class AutoDrop
		{
		public:
			AutoDrop(Manager* pManager, pid_t id) : m_pManager(pManager), m_id(id)
			{}

			~AutoDrop();

			pid_t detach()
			{
				pid_t id = m_id;
				m_id = 0;
				return id;
			}

		private:
			Manager* m_pManager;
			pid_t    m_id;
		};
	};
}

#endif // OOSERVER_ROOT_CLIENT_CONN_H_INCLUDED_
