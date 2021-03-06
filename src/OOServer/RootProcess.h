///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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

#ifndef OOSERVER_ROOT_PROCESS_H_INCLUDED_
#define OOSERVER_ROOT_PROCESS_H_INCLUDED_

#include "Protocol.h"

namespace Root
{
	class Manager;

	class Process
	{
	public:
		virtual ~Process() {}

		virtual int CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed) const = 0;
		virtual bool IsSameLogin(OOBase::AsyncLocalSocket::uid_t uid, const char* session_id) const = 0;
		virtual bool IsSameUser(OOBase::AsyncLocalSocket::uid_t uid) const = 0;

		virtual bool IsRunning() const = 0;
		virtual OOServer::RootErrCode LaunchService(Root::Manager* pManager, const OOBase::LocalString& strName, const Omega::int64_t& key, unsigned long wait_secs, bool async, OOBase::RefPtr<OOBase::Socket>& ptrSocket) const = 0;

	protected:
		Process() {}

	private:
		Process(const Process&);
		Process& operator = (const Process&);
	};
}

#endif // OOSERVER_ROOT_PROCESS_H_INCLUDED_

