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

#include "OOServer_User.h"
#include "UserProcess.h"

#if defined(HAVE_UNISTD_H)

namespace
{
	class UserProcessUnix : public User::Process
	{
	public:
		virtual bool running();
		virtual bool wait_for_exit(const OOBase::timeval_t* wait, int* exit_code);

		void exec(const std::wstring& strExeName);

	private:

	};
}

User::Process* User::Process::exec(const std::wstring& strExeName)
{
	OOBase::SmartPtr<UserProcessUnix> ptrProcess;
	OOBASE_NEW(ptrProcess,UserProcessUnix());
	if (!ptrProcess)
		OMEGA_THROW(ENOMEM);

	ptrProcess->exec(strExeName);
	return ptrProcess.detach();
}

void UserProcessUnix::exec(const std::wstring& strExeName)
{
	void* POSIX_TODO;
}

bool UserProcessUnix::running()
{
	void* POSIX_TODO;

	return false;
}

bool UserProcessUnix::wait_for_exit(const OOBase::timeval_t* wait, int* exit_code)
{
	void* POSIX_TODO;

	return false;
}

#endif // HAVE_UNISTD_H
