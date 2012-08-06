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
		UserProcessUnix() : m_pid(0), m_fd(-1)
		{}

		~UserProcessUnix()
		{
			OOBase::POSIX::close(m_fd);
		}

		bool is_running(int& exit_code);
		void kill();
		void exec(const char* pszExeName, const char* pszWorkingDir, char** env);

	private:
		pid_t m_pid;
		int   m_fd;
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

	pid_t safe_wait_pid(pid_t pid, int* status, int options)
	{
		for (;;)
		{
			pid_t ret = ::waitpid(pid,status,options);
			if (ret != -1)
				return ret;

			if (errno != EINTR)
				OMEGA_THROW(errno);
		}
	}
}

bool User::Process::is_invalid_path(const Omega::string_t& strPath)
{
	// Valid paths can contain PATH-based paths
	return (strPath[0] != '/' && strPath.Find('/') != Omega::string_t::npos);
}

void UserProcessUnix::exec(const char* pszExeName, const char* pszWorkingDir, char** env)
{
	// Create a pipe() pair, and wait for the child to close the write end
	int fd[2] = {-1,-1};
	int err = pipe(fd);
	if (err)
		OMEGA_THROW(errno);

	// We want the file to close on exec (during the system() call below)
	err = OOBase::POSIX::set_close_on_exec(fd[1],true);
	if (err)
	{
		OOBase::POSIX::close(fd[0]);
		OOBase::POSIX::close(fd[1]);
		OMEGA_THROW(err);
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		OOBase::POSIX::close(fd[0]);
		OOBase::POSIX::close(fd[1]);
		OMEGA_THROW(errno);
	}

	if (pid != 0)
	{
		// We are the parent
		OOBase::POSIX::close(fd[1]);
		m_fd = fd[0];
		m_pid = pid;
		return;
	}

	// We are the child...

	// Close all open handles except the standard ones and fd[1]
	int except[] = { STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO, fd[1] };
	err = OOBase::POSIX::close_file_descriptors(except,sizeof(except)/sizeof(except[0]));
	if (err)
		exit_msg("close_file_descriptors() failed: %s\n",OOBase::system_error_text(err));

	// Reset the environment
	::environ = env;

	// Set cwd
	if (pszWorkingDir)
	{
		int err = ::chdir(pszWorkingDir);
		if (err)
			exit_msg("chdir(%s): %s\n",pszWorkingDir,OOBase::system_error_text(err));
	}

#if 0
	OOBase::LocalString valgrind;
	valgrind.printf("xterm -T '%s' -e 'libtool --mode=execute valgrind --leak-check=full --log-file=valgrind_log%d.txt %s'",pszExeName,getpid(),pszExeName);
	execlp("/bin/sh","sh","-c",valgrind.c_str(),(char*)NULL);
#endif

	if (execlp("/bin/sh","sh","-c",pszExeName,(char*)NULL) == -1)
	{
		err = errno;
		exit_msg("execlp(/bin/sh -c %s): %s\n",pszExeName,OOBase::system_error_text(err));
	}
}

bool UserProcessUnix::is_running(int& exit_code)
{
	if (m_pid != 0)
	{
		pid_t retv = safe_wait_pid(m_pid,&exit_code,WNOHANG);
		if (retv == 0)
			return true;

		m_pid = 0;
	}

	return false;
}

void UserProcessUnix::kill()
{
	if (m_pid != 0)
	{
		::kill(m_pid,SIGKILL);

		safe_wait_pid(m_pid,NULL,0);

		m_pid = 0;
	}
}

User::Process* User::Manager::exec(const Omega::string_t& strExeName, const Omega::string_t& strWorkingDir, bool /*is_host_process*/, const OOBase::Table<OOBase::String,OOBase::String,OOBase::LocalAllocator>& tabEnv)
{
	OOBase::SmartPtr<UserProcessUnix> ptrProcess = new (std::nothrow) UserProcessUnix();
	if (!ptrProcess)
		OMEGA_THROW(ENOMEM);

	OOBase::Logger::log(OOBase::Logger::Information,"Executing process %s",strExeName.c_str());

	ptrProcess->exec(strExeName.c_str(),strWorkingDir.IsEmpty() ? NULL : strWorkingDir.c_str(),OOBase::Environment::get_envp(tabEnv));
	return ptrProcess.detach();
}

#endif // HAVE_UNISTD_H
