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

		void exec(const wchar_t* app_name, wchar_t* cmd_line, const wchar_t* working_dir, LPVOID env_block);

	private:
		OOBase::Win32::SmartHandle m_hProcess;
	};

	template <typename T>
	OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> to_wchar_t(const T& str)
	{
		OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> wsz;
		int len = MultiByteToWideChar(CP_UTF8,0,str.c_str(),-1,NULL,0);
		if (len == 0)
		{
			DWORD dwErr = GetLastError();
			if (dwErr != ERROR_INSUFFICIENT_BUFFER)
				OMEGA_THROW(dwErr);
		}

		wsz = static_cast<wchar_t*>(OOBase::LocalAllocator::allocate((len+1) * sizeof(wchar_t)));
		if (!wsz)
			OMEGA_THROW(GetLastError());
		
		MultiByteToWideChar(CP_UTF8,0,str.c_str(),-1,wsz,len);
		wsz[len] = L'\0';
		return wsz;
	}
}

bool User::Process::is_invalid_path(const Omega::string_t& strPath)
{
	return (PathIsRelativeW(to_wchar_t(strPath)) != FALSE);
}

void UserProcessWin32::exec(const wchar_t* app_name, wchar_t* cmd_line, const wchar_t* working_dir, LPVOID env_block)
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
	if (!CreateProcessW(app_name,cmd_line,NULL,NULL,FALSE,dwFlags,env_block,working_dir,&si,&pi))
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

User::Process* User::Manager::exec(const Omega::string_t& strExeName, const Omega::string_t& strWorkingDir, bool is_host_process, const OOBase::Table<OOBase::String,OOBase::String,OOBase::LocalAllocator>& tabEnv)
{
	OOBase::SmartPtr<UserProcessWin32> ptrProcess = new (std::nothrow) UserProcessWin32();
	if (!ptrProcess)
		OMEGA_THROW(ERROR_OUTOFMEMORY);

	Omega::string_t strProcess;
	get_root_config_arg("binary_path",strProcess);
	strProcess += "OOSvrHost.exe";

	OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> cmd_line;
	if (!is_host_process)
		cmd_line = to_wchar_t(" --shellex -- " + strExeName);
	else
		cmd_line = to_wchar_t(strExeName);
	
	ptrProcess->exec(to_wchar_t(strProcess),cmd_line,strWorkingDir.IsEmpty() ? NULL : to_wchar_t(strWorkingDir),OOBase::Environment::get_block(tabEnv));
	return ptrProcess.detach();
}

#endif // _WIN32
