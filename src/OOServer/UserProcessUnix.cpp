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
		virtual bool wait_for_exit(const OOBase::timeval_t* wait, int& exit_code);

		void exec(const wchar_t* pszExeName, OOBase::Set<Omega::string_t,OOBase::LocalAllocator>& env);

	private:
		pid_t m_pid;
	};
}

bool User::Process::is_relative_path(const wchar_t* pszPath)
{
	return (pszPath[0] != L'/');
}

User::Process* User::Process::exec(const wchar_t* pszExeName, OOBase::Set<Omega::string_t,OOBase::LocalAllocator>& env)
{
	OOBase::SmartPtr<UserProcessUnix> ptrProcess = new (std::nothrow) UserProcessUnix();
	if (!ptrProcess)
		OMEGA_THROW(ENOMEM);

	ptrProcess->exec(pszExeName,env);
	return ptrProcess.detach();
}

void UserProcessUnix::exec(const wchar_t* pszExeName, OOBase::Set<Omega::string_t,OOBase::LocalAllocator>& env)
{
	pid_t pid = fork();
	if (pid < 0)
		OMEGA_THROW(errno);

	if (pid == 0)
	{
		// We are the child

		// Sort out environment block and split args

		// We need to set the LC_CTYPE before the conversion...
		void* TODO;
		/*if (env_var(LC_ALL))
			setlocale(LC_ALL,v);
		else
		{
			if (env_var(LANG))
				setlocale(LC_ALL,v);
			else
				setlocale(LC_ALL,"C");

			if (env_var(LC_CTYPE))
				set = (setlocale(LC_CTYPE,v) != NULL);
		}*/

		char szBuf[1024] = {0};
		size_t clen = OOBase::to_native(szBuf,sizeof(szBuf),pszExeName,size_t(-1));
		if (clen >= sizeof(szBuf))
		{
			OOBase::stderr_write("exec filename > 1024\n");
			_exit(127);
		}
		szBuf[clen] = '\0';

		// Use execve()

		// Just use the system() call
		int ret = system(szBuf);
		if (WIFEXITED(ret))
			_exit(WEXITSTATUS(ret));

		OOBase::stderr_write("Failed to launch process\n");
		_exit(127);
	}
	else
	{
		if (User::is_debug())
			AttachDebugger(pid);
	}

	m_pid = pid;
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

bool UserProcessUnix::wait_for_exit(const OOBase::timeval_t* wait, int& exit_code)
{
	if (m_pid == 0)
		return true;

	if (wait)
		OOBase::Thread::sleep(*wait);

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

	return false;
}

#endif // HAVE_UNISTD_H
