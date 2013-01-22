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

		bool recv_next();

		void on_message_start(OOBase::CDRStream& request, int err);

#if defined(HAVE_UNISTD_H)
		OOBase::POSIX::SmartFD         m_passed_fd;

		void on_message_posix_1(OOBase::Buffer* data_buffer, OOBase::Buffer* ctl_buffer, int err);
		void on_message_posix_2(OOBase::Buffer* data_buffer, int err);
#elif defined(_WIN32)
		void on_message_win32_1(OOBase::Buffer* data_buffer, int err);
		void on_message_win32_2(OOBase::Buffer* data_buffer, int err);
#else
#error Implement platform native credential and pipe handle passing
#endif

	};
}

#endif // OOSERVER_REGISTRY_ROOT_CONN_H_INCLUDED_
