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

#ifndef OOSERVER_ROOT_USER_CONN_H_INCLUDED_
#define OOSERVER_ROOT_USER_CONN_H_INCLUDED_

namespace Root
{
	class UserConnection : public OOBase::RefCounted
	{
	public:
		UserConnection(Manager* pManager, OOBase::SmartPtr<Process>& ptrProcess, OOBase::RefPtr<OOBase::AsyncSocket>& ptrSocket);

		bool same_login(const uid_t& uid, const char* session_id) const;
		const uid_t& get_uid() const;
		pid_t get_pid() const;

#if defined(_WIN32)
		bool start(pid_t client_id, const OOBase::String& fd_root);
		bool start(pid_t client_id, const OOBase::String& fd_user, const OOBase::String& fd_root);
		bool do_start(pid_t client_id, const OOBase::String& fd_user, const OOBase::String& fd_root, const OOBase::String& fd_sbox);
		bool on_add_client(OOBase::CDRStream& response, pid_t client_id);
		struct user_params_t
		{
			pid_t client_id;
			pid_t user_id;
			OOBase::String strUserFd;
			OOBase::String strRootFd;
		};
		bool on_start(OOBase::CDRStream& response, const user_params_t& params);
#elif defined(HAVE_UNISTD_H)
		bool start(pid_t client_id, OOBase::POSIX::SmartFD& fd_root);
		bool start(pid_t client_id, OOBase::POSIX::SmartFD& fd_user, OOBase::POSIX::SmartFD& fd_root);
		bool do_start(pid_t client_id, OOBase::POSIX::SmartFD& fd_user, OOBase::POSIX::SmartFD& fd_root, OOBase::POSIX::SmartFD& fd_sandbox);
		bool on_add_client(OOBase::CDRStream& stream, pid_t client_id, int client_fd);
		struct user_params_t
		{
			pid_t client_id;
			pid_t user_id;
			int user_fd;
			int root_fd;
			int sbox_fd;
		};
		bool on_start(OOBase::CDRStream& response, const user_params_t& params);
#endif
		bool add_client(pid_t client_id);

	private:
		Manager*                            m_pManager;
		OOBase::SmartPtr<Process>           m_ptrProcess;
		OOBase::RefPtr<OOBase::AsyncSocket> m_ptrSocket;

		OOBase::AsyncResponseDispatcher<Omega::uint16_t> m_async_dispatcher;

		bool start();
		bool on_started(OOBase::CDRStream& stream, pid_t client_id);

#if defined(_WIN32)
		bool new_connection(pid_t client_id, OOBase::AsyncResponseDispatcher<Omega::uint16_t>::AutoDrop& response_id);
#elif defined(HAVE_UNISTD_H)
		bool new_connection(pid_t client_id, OOBase::POSIX::SmartFD& fd, OOBase::AsyncResponseDispatcher<Omega::uint16_t>::AutoDrop& response_id);
		void on_sent_msg(OOBase::Buffer* data, OOBase::Buffer* ctl, int err);
#endif
		void on_sent(OOBase::Buffer* buffer, int err);
		void on_response(OOBase::Buffer* buffer, int err);

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

#endif // OOSERVER_ROOT_USER_CONN_H_INCLUDED_
