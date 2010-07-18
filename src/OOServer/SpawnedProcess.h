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
//  Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_SPAWNED_PROCESS_H_INCLUDED_
#define OOSERVER_SPAWNED_PROCESS_H_INCLUDED_

namespace Root
{
	class SpawnedProcess
	{
	public:
		virtual ~SpawnedProcess() {}

		virtual bool CheckAccess(const char* pszFName, bool bRead, bool bWrite, bool& bAllowed) const = 0;
		virtual bool Compare(OOSvrBase::AsyncLocalSocket::uid_t uid) const = 0;
		virtual bool IsSameUser(OOSvrBase::AsyncLocalSocket::uid_t uid) const = 0;
		virtual bool GetRegistryHive(const std::string& strSysDir, const std::string& strUsersDir, std::string& strHive) = 0;

	protected:
		SpawnedProcess() {}

	private:
		SpawnedProcess(const SpawnedProcess&);
		SpawnedProcess& operator = (const SpawnedProcess&);
	};
}

#endif // OOSERVER_SPAWNED_PROCESS_H_INCLUDED_

