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
#include "UserManager.h"
#include "UserProcess.h"

#if defined(HAVE_UNISTD_H)

#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

void AttachDebugger(unsigned long pid);

namespace
{
	class UserProcessUnix : public User::Process
	{
	public:
		UserProcessUnix() : m_pid(0)
		{}

		virtual bool running();
		virtual bool wait_for_exit(const OOBase::Timeout& timeout, int& exit_code);

		void exec(const Omega::string_t& strExeName, OOBase::Set<Omega::string_t,OOBase::LocalAllocator>& env);

	private:
		pid_t m_pid;
	};
}

bool User::Process::is_relative_path(const Omega::string_t& strPath)
{
	return (strPath[0] != '/');
}

User::Process* User::Process::exec(const Omega::string_t& strExeName, OOBase::Set<Omega::string_t,OOBase::LocalAllocator>& env)
{
	OOBase::SmartPtr<UserProcessUnix> ptrProcess = new (std::nothrow) UserProcessUnix();
	if (!ptrProcess)
		OMEGA_THROW(ENOMEM);

	ptrProcess->exec(strExeName,env);
	return ptrProcess.detach();
}

void UserProcessUnix::exec(const Omega::string_t& strExeName, OOBase::Set<Omega::string_t,OOBase::LocalAllocator>& env)
{
	const char* pszExeName = strExeName.c_str();

	pid_t pid = fork();
	if (pid < 0)
		OMEGA_THROW(errno);

	if (pid != 0)
	{
		if (User::is_debug())
			AttachDebugger(pid);

		m_pid = pid;
		return;
	}

	// We are the child

	// Sort out environment block and split args

	// Update PWD?

	// Use execve()

	// Just use the system() call
	int ret = system(pszExeName);
	if (WIFEXITED(ret))
		_exit(WEXITSTATUS(ret));

	OOBase::stderr_write("Failed to launch process\n");
	_exit(127);
}

bool UserProcessUnix::running()
{
	if (m_pid == 0)
		return false;

	pid_t retv = waitpid(m_pid,NULL,WNOHANG);
	if (retv == 0)
		return true;

	m_pid = 0;
	return false;
}

bool UserProcessUnix::wait_for_exit(const OOBase::Timeout& timeout, int& exit_code)
{
	if (m_pid == 0)
		return true;

	while (!timeout.has_expired())
	{
		int status = 0;
		pid_t retv = waitpid(m_pid,&status,WNOHANG);
		if (retv != 0)
		{
			if (WIFEXITED(status))
				exit_code = WEXITSTATUS(status);
			else
				exit_code = -1;
			return true;
		}

		OOBase::Thread::sleep(0);
	}

	return false;
}

#endif // HAVE_UNISTD_H
