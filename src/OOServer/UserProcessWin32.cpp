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

#if defined(_MSC_VER)
#define wcsicmp _wcsicmp
#endif

namespace
{
	class UserProcessWin32 : public User::Process
	{
	public:
		virtual bool is_running(int& exit_code);
		virtual void kill();

		void exec(const wchar_t* app_name, wchar_t* cmd_line, const wchar_t* working_dir, LPVOID env_block);

	private:
		OOBase::Win32::SmartHandle m_hProcess;
	};
}

bool User::Process::is_invalid_path(const Omega::string_t& strPath)
{
	OOBase::ScopedArrayPtr<wchar_t> path;
	int err = OOBase::Win32::utf8_to_wchar_t(strPath.c_str(),path);
	if (err)
		OMEGA_THROW(err);

	return (PathIsRelativeW(path.get()) != FALSE);
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
		OOBase::Win32::AttachDebugger(pi.dwProcessId);

	if (hDebugEvent)
		SetEvent(hDebugEvent);

	CloseHandle(pi.hThread);
	m_hProcess = pi.hProcess;
}

bool UserProcessWin32::is_running(int& exit_code)
{
	if (!m_hProcess.is_valid())
		return false;

	DWORD dwWait = WaitForSingleObject(m_hProcess,0);
	if (dwWait == WAIT_TIMEOUT)
		return true;

	if (dwWait != WAIT_OBJECT_0)
		OOBase_CallCriticalFailure(GetLastError());
	
	DWORD dwCode;
	if (GetExitCodeProcess(m_hProcess,&dwCode))
		exit_code = dwCode;
	else
		exit_code = -1;

	m_hProcess.close();
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

OOBase::SharedPtr<User::Process> User::Manager::exec(const Omega::string_t& strExeName, const Omega::string_t& strWorkingDir, bool is_host_process, const OOBase::Environment::env_table_t& tabEnv)
{
	OOBase::SharedPtr<UserProcessWin32> ptrProcess = OOBase::allocate_shared<UserProcessWin32,OOBase::CrtAllocator>();
	if (!ptrProcess)
		throw Omega::ISystemException::OutOfMemory();

	Omega::string_t strProcess;
	get_root_config_arg("binary_path",strProcess);

#if defined(WIN64_HYBRID)
	strProcess += "OOSvrHost64.exe";
#else
	strProcess += "OOSvrHost32.exe";
#endif

	int err = 0;
	OOBase::ScopedArrayPtr<wchar_t> cmd_line;
	if (!is_host_process)
	{
		OOBase::Logger::log(OOBase::Logger::Information,"Executing process %s",strExeName.c_str());

		err = OOBase::Win32::utf8_to_wchar_t((" --shellex -- " + strExeName).c_str(),cmd_line);
		if (err)
			OMEGA_THROW(err);
	}
	else
	{
		OOBase::Logger::log(OOBase::Logger::Information,"Executing process %s",strProcess.c_str());

		err = OOBase::Win32::utf8_to_wchar_t(strExeName.c_str(),cmd_line);
		if (err)
			OMEGA_THROW(err);
	}
	
	OOBase::ScopedArrayPtr<wchar_t> wd;
	if (!strWorkingDir.IsEmpty())
	{
		err = OOBase::Win32::utf8_to_wchar_t(strWorkingDir.c_str(),wd);
		if (err)
			OMEGA_THROW(err);
	}

	OOBase::ScopedArrayPtr<wchar_t> env_block;
	err = OOBase::Environment::get_block(tabEnv,env_block);
	if (err)
		OMEGA_THROW(err);

	OOBase::ScopedArrayPtr<wchar_t> exe;
	err = OOBase::Win32::utf8_to_wchar_t(strProcess.c_str(),exe);
	if (err)
		OMEGA_THROW(err);

	ptrProcess->exec(exe.get(),cmd_line.get(),wd.get(),env_block.get());
	return OOBase::static_pointer_cast<Process>(ptrProcess);
}

#endif // _WIN32
