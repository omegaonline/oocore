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

namespace
{
	class UserProcessUnix : public User::Process
	{
	public:
		UserProcessUnix() : m_pid(0)
		{}

		virtual bool running();
		virtual bool wait_for_exit(const OOBase::Timeout& timeout, int& exit_code);
		virtual void kill();

		void exec(const Omega::string_t& strExeName, char** env);

	private:
		pid_t m_pid;
	};

	void exit_msg(const char* fmt, ...)
	{
		va_list args;
		va_start(args,fmt);

		OOBase::LocalString msg;
		int err = msg.vprintf(fmt,args);

		va_end(args);

		if (err == 0)
			OOBase::stderr_write(msg.c_str());

		_exit(127);
	}
}

bool User::Process::is_invalid_path(const Omega::string_t& strPath)
{
	// Valid paths can contain PATH-based paths
	return (strPath[0] != '/' && strPath.Find('/') != Omega::string_t::npos);
}

User::Process* User::Process::exec(const Omega::string_t& strExeName, bool /*is_surrogate*/, const OOBase::Table<OOBase::String,OOBase::String,OOBase::LocalAllocator>& tabEnv)
{
	OOBase::SmartPtr<UserProcessUnix> ptrProcess = new (std::nothrow) UserProcessUnix();
	if (!ptrProcess)
		OMEGA_THROW(ENOMEM);

	ptrProcess->exec(strExeName,OOBase::Environment::get_envp(tabEnv));
	return ptrProcess.detach();
}

void UserProcessUnix::exec(const Omega::string_t& strExeName, char** env)
{
	// Create a pipe() pair, and wait for the write end to close in the child
	void* TODO;

	pid_t pid = fork();
	if (pid < 0)
		OMEGA_THROW(errno);

	if (pid != 0)
	{
		// We are the parent
		m_pid = pid;
		return;
	}

	// We are the child

	// Close all open handles except the standard ones
	int except[] = { STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO };
	int err = OOBase::POSIX::close_file_descriptors(except,sizeof(except)/sizeof(except[0]));
	if (err)
		exit_msg("close_file_descriptors() failed: %s\n",OOBase::system_error_text(err));

	// Reset the environment
	::environ = env;

	// Just use the system() call
	err = system(strExeName.c_str());
	if (!WIFEXITED(err))
		exit_msg("Failed to launch process %s\n",strExeName.c_str());

	_exit(WEXITSTATUS(err));
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

void UserProcessUnix::kill()
{
	if (running())
	{
		::kill(m_pid,SIGKILL);

		int status = 0;
		waitpid(m_pid,&status,0);

		m_pid = 0;
	}
}

#endif // HAVE_UNISTD_H
