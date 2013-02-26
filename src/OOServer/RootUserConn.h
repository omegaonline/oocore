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
		bool start(pid_t client_id, const OOBase::String& fd_user, const OOBase::String& fd_root);
#elif defined(HAVE_UNISTD_H)
		bool start(pid_t client_id, OOBase::POSIX::SmartFD& fd_user, OOBase::POSIX::SmartFD& fd_root);
#endif

		bool add_client(OOBase::RefPtr<ClientConnection>& ptrClient);

	private:
		Manager*                            m_pManager;
		OOBase::SmartPtr<Process>           m_ptrProcess;
		OOBase::RefPtr<OOBase::AsyncSocket> m_ptrSocket;

#if defined(HAVE_UNISTD_H)
		void on_sent_msg(OOBase::Buffer* data, OOBase::Buffer* ctl, int err);
#endif
		void on_sent(OOBase::Buffer* buffer, int err);
	};
}

#endif // OOSERVER_ROOT_USER_CONN_H_INCLUDED_
