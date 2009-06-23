///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
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

#ifndef OOSERVER_USER_ACCEPTOR_H_INCLUDED_
#define OOSERVER_USER_ACCEPTOR_H_INCLUDED_

#include "OOServer_User.h"

#include "../OOBase/SecurityWin32.h"

namespace User
{
	class Manager;

	class Acceptor : public OOSvrBase::Acceptor
	{
	public:
		Acceptor();
		virtual ~Acceptor();

		bool start(Manager* pManager, const std::string& pipe_name);
		void stop();

		bool on_accept(OOBase::Socket* pSocket, int err);

		static std::string unique_name();

	private:
		Acceptor(const Acceptor&) {}
		Acceptor& operator = (const Acceptor&) { return *this; }

		Manager*        m_pManager;
		OOBase::Socket* m_pSocket;

		bool init_security(const std::string& pipe_name);

		SECURITY_ATTRIBUTES              m_sa;

#if defined(_WIN32)
		OOSvrBase::Win32::sec_descript_t m_sd;
#endif
	};
}

#endif // OOSERVER_USER_ACCEPTOR_H_INCLUDED_
