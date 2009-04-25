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

/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_CLIENT_ACCEPTOR_H_INCLUDED_
#define OOSERVER_CLIENT_ACCEPTOR_H_INCLUDED_

#include "OOServer_Root.h"

#include "../OOBase/SecurityWin32.h"

namespace Root
{
	class Manager;

	class ClientAcceptor : public OOSvrBase::Acceptor
	{
	public:
		ClientAcceptor();
		virtual ~ClientAcceptor();

		bool start(Manager* pManager);
		void stop();

		bool on_accept(OOBase::Socket* pSocket, int err);

	private:
		ClientAcceptor(const ClientAcceptor&) {}
		ClientAcceptor& operator = (const ClientAcceptor&) { return *this; }

		Manager*        m_pManager;
		OOBase::Socket* m_pSocket;
		std::string     m_pipe_name;

		bool init_security(const std::string& pipe_name);

#if defined(_WIN32)
		SECURITY_ATTRIBUTES              m_sa;
		OOSvrBase::Win32::sec_descript_t m_sd;
#endif
	};
}

#endif // OOSERVER_CLIENT_ACCEPTOR_H_INCLUDED_
