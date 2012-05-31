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

#if defined(_WIN32)

#include <shlwapi.h>

void AttachDebugger(DWORD pid);

#if defined(_MSC_VER)
#define wcsicmp _wcsicmp
#endif

namespace
{
	class UserProcessWin32 : public User::Process
	{
	public:
		virtual bool running();
		virtual bool wait_for_exit(const OOBase::Timeout& timeout, int& exit_code);
		virtual void kill();

		void exec(wchar_t* cmd_line, const wchar_t* working_dir, LPVOID env_block);

	private:
		OOBase::Win32::SmartHandle m_hProcess;
	};
}

bool User::Process::is_invalid_path(const Omega::string_t& strPath)
{
	wchar_t wpath[MAX_PATH] = {0};
	if (MultiByteToWideChar(CP_UTF8,0,strPath.c_str(),-1,wpath,MAX_PATH-1) <= 0)
		return true;

	return (PathIsRelativeW(wpath) != FALSE);
}

User::Process* User::Process::exec(const Omega::string_t& strExeName, const Omega::string_t& strWorkingDir, bool is_host_process, const OOBase::Table<OOBase::String,OOBase::String,OOBase::LocalAllocator>& tabEnv)
{
	OOBase::SmartPtr<UserProcessWin32> ptrProcess = new (std::nothrow) UserProcessWin32();
	if (!ptrProcess)
		OMEGA_THROW(ERROR_OUTOFMEMORY);

	// Get OOSvrHost to run ShellExecuteEx!!
	void* TODO;

	wchar_t cmd_line[MAX_PATH] = {0};
	if (MultiByteToWideChar(CP_UTF8,0,strExeName.c_str(),-1,cmd_line,MAX_PATH-1) <= 0)
		OMEGA_THROW(GetLastError());

	wchar_t working_dir_buf[MAX_PATH] = {0};
	const wchar_t* working_dir = NULL;
	if (!strWorkingDir.IsEmpty())
	{
		if (MultiByteToWideChar(CP_UTF8,0,strWorkingDir.c_str(),-1,working_dir_buf,MAX_PATH-1) <= 0)
			OMEGA_THROW(GetLastError());

		working_dir = working_dir_buf;
	}

	// Do a ShellExecute style lookup for the actual thing to call..
	ptrProcess->exec(cmd_line,working_dir,OOBase::Environment::get_block(tabEnv));
	return ptrProcess.detach();
}

void UserProcessWin32::exec(wchar_t* cmd_line, const wchar_t* working_dir, LPVOID env_block)
{
	DWORD dwFlags = DETACHED_PROCESS;

	OOBase::Win32::SmartHandle hDebugEvent;
	if (User::is_debug())
	{
		hDebugEvent = CreateEventW(NULL,FALSE,FALSE,L"Local\\OOCORE_DEBUG_MUTEX");

		dwFlags = CREATE_NEW_CONSOLE;
	}

	dwFlags |= CREATE_UNICODE_ENVIRONMENT;

	STARTUPINFOW si = {0};
	si.cb = sizeof(STARTUPINFOW);

	// Spawn the process
	PROCESS_INFORMATION pi = {0};
	if (!CreateProcessW(NULL,cmd_line,NULL,NULL,FALSE,dwFlags,env_block,working_dir,&si,&pi))
	{
		DWORD dwErr = GetLastError();
		OMEGA_THROW(dwErr);
	}

	if (User::is_debug())
		AttachDebugger(pi.dwProcessId);

	if (hDebugEvent)
		SetEvent(hDebugEvent);

	CloseHandle(pi.hThread);
	m_hProcess = pi.hProcess;
}

bool UserProcessWin32::running()
{
	if (!m_hProcess.is_valid())
		return false;

	DWORD dwWait = WaitForSingleObject(m_hProcess,0);
	if (dwWait == WAIT_TIMEOUT)
		return true;

	if (dwWait != WAIT_OBJECT_0)
		OOBase_CallCriticalFailure(GetLastError());

	return false;
}

bool UserProcessWin32::wait_for_exit(const OOBase::Timeout& timeout, int& exit_code)
{
	if (!m_hProcess.is_valid())
		return true;

	DWORD dwWait = WaitForSingleObject(m_hProcess,timeout.millisecs());
	if (dwWait == WAIT_OBJECT_0)
	{
		DWORD dwCode;
		if (GetExitCodeProcess(m_hProcess,&dwCode))
			exit_code = dwCode;
		else
			exit_code = -1;

		return true;
	}
	
	if (dwWait != WAIT_TIMEOUT)
		OOBase_CallCriticalFailure(GetLastError());

	return false;
}

void UserProcessWin32::kill()
{
	if (m_hProcess.is_valid())
	{
		TerminateProcess(m_hProcess,127);

		WaitForSingleObject(m_hProcess,INFINITE);
	}
}

#endif // _WIN32
