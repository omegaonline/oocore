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

namespace
{
	class UserProcessUnix : public User::Process
	{
	public:
		UserProcessUnix() : m_pid(0)
		{}

		bool is_running(int& exit_code);
		void kill();
		void exec(const char* pszExeName, const char* pszWorkingDir, char** env);

	private:
		pid_t m_pid;
	};

	void exit_msg(OOBase::AllocatorInstance& allocator, const char* fmt, ...)
	{
		va_list args;
		va_start(args,fmt);

		OOBase::TempPtr<char> msg(allocator);
		int err = OOBase::vprintf(msg,fmt,args);

		va_end(args);

		if (err == 0)
			OOBase::stderr_write(msg);

		_exit(127);
	}
}

bool User::Process::is_invalid_path(const Omega::string_t& strPath)
{
	// Valid paths can contain PATH-based paths
	return (strPath[0] != '/' && strPath.Find('/') != Omega::string_t::npos);
}

void UserProcessUnix::exec(const char* pszExeName, const char* pszWorkingDir, char** env)
{
	pid_t pid = fork();
	if (pid < 0)
		OMEGA_THROW(errno);

	if (pid != 0)
	{
		// We are the parent
		m_pid = pid;
		return;
	}

	// We are the child...

	// Need a new stack
	OOBase::StackAllocator<512> allocator;

	// Close all open handles except the standard ones and fd[1]
	int except[] = { STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO };
	int err = OOBase::POSIX::close_file_descriptors(except,sizeof(except)/sizeof(except[0]));
	if (err)
		exit_msg(allocator,"close_file_descriptors() failed: %s\n",OOBase::system_error_text(err));

	// Reset the environment
	::environ = env;

	// Set cwd
	if (pszWorkingDir)
	{
		int err = ::chdir(pszWorkingDir);
		if (err)
			exit_msg(allocator,"chdir(%s): %s\n",pszWorkingDir,OOBase::system_error_text(err));
	}

	if (User::is_debug())
	{
		OOBase::LocalString display(allocator);
		OOBase::Environment::getenv("DISPLAY",display);
		if (!display.empty())
		{
#if 0
			OOBase::LocalString valgrind(allocator);
			valgrind.printf("xterm -T '%s' -e 'libtool --mode=execute valgrind --leak-check=full --log-file=valgrind_log%d.txt %s'",pszExeName,getpid(),pszExeName);
			execlp("/bin/sh","sh","-c",valgrind.c_str(),(char*)NULL);
#endif

			OOBase::LocalString ex(allocator);
			ex.printf("xterm -T '%s' -e '%s'",pszExeName,pszExeName);
			execlp("/bin/sh","sh","-c",ex.c_str(),(char*)NULL);
		}
	}

	execlp("/bin/sh","sh","-c",pszExeName,(char*)NULL);

	err = errno;
	exit_msg(allocator,"execlp(/bin/sh -c %s): %s\n",pszExeName,OOBase::system_error_text(err));
}

bool UserProcessUnix::is_running(int& exit_code)
{
	if (m_pid != 0)
	{
		pid_t retv = OOBase::POSIX::waitpid(m_pid,&exit_code,WNOHANG);
		if (retv == 0)
			return true;

		if (retv == -1)
			LOG_ERROR(("waitpid() failed: %s",OOBase::system_error_text()));

		m_pid = 0;
	}

	return false;
}

void UserProcessUnix::kill()
{
	if (m_pid != 0)
	{
		::kill(m_pid,SIGKILL);

		pid_t retv = OOBase::POSIX::waitpid(m_pid,NULL,0);
		if (retv == -1)
			LOG_ERROR(("waitpid() failed: %s",OOBase::system_error_text()));

		m_pid = 0;
	}
}

User::Process* User::Manager::exec(const Omega::string_t& strExeName, const Omega::string_t& strWorkingDir, bool /*is_host_process*/, const OOBase::Environment::env_table_t& tabEnv)
{
	OOBase::SmartPtr<UserProcessUnix> ptrProcess = new (std::nothrow) UserProcessUnix();
	if (!ptrProcess)
		OMEGA_THROW(ENOMEM);

	OOBase::TempPtr<char*> ptrEnv(tabEnv.get_allocator());
	int err = OOBase::Environment::get_envp(tabEnv,ptrEnv);
	if (err)
		OMEGA_THROW(err);

	OOBase::Logger::log(OOBase::Logger::Information,"Executing process %s",strExeName.c_str());

	ptrProcess->exec(strExeName.c_str(),strWorkingDir.IsEmpty() ? NULL : strWorkingDir.c_str(),ptrEnv);
	return ptrProcess.detach();
}

#endif // HAVE_UNISTD_H
