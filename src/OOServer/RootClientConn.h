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

	private:
		Manager*                            m_pManager;
		OOBase::RefPtr<OOBase::AsyncSocket> m_socket;

		pid_t  m_pid;
		uid_t  m_uid;

#if defined(HAVE_UNISTD_H)
		void on_message_posix(OOBase::CDRStream& stream, OOBase::Buffer* ctl_buffer, int err);
#endif

		void on_message(OOBase::CDRStream& stream, int err);
	};
}

#endif // OOSERVER_ROOT_CLIENT_CONN_H_INCLUDED_
