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

void AttachDebugger(unsigned long pid);

#if defined(_MSC_VER)
#define wcsicmp _wcsicmp
#endif

namespace
{
	class UserProcessWin32 : public User::Process
	{
	public:
		virtual bool running();
		virtual bool wait_for_exit(const OOBase::timeval_t* wait, int& exit_code);

		void exec(OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> ptrCmdLine);

	private:
		OOBase::Win32::SmartHandle m_hProcess;
	};

	static OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> CopyCmdLine(const wchar_t* psz)
	{
		size_t wlen = (wcslen(psz)+1)*sizeof(wchar_t);

		OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> ptrCmdLine;
		if (!ptrCmdLine.allocate(wlen))
			OMEGA_THROW(ERROR_OUTOFMEMORY);

		memcpy(ptrCmdLine,psz,wlen);

		return ptrCmdLine;
	}

	static OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> ShellParse(const wchar_t* pszFile)
	{
		OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> ptrCmdLine = CopyCmdLine(pszFile);
		
		const wchar_t* pszExt = PathFindExtensionW(pszFile);
		if (pszExt && wcsicmp(pszExt,L".exe")!=0)
		{
			DWORD dwLen = 1024;
			wchar_t szBuf[1024];
			ASSOCF flags = (ASSOCF)(ASSOCF_NOTRUNCATE | ASSOCF_REMAPRUNDLL);
			HRESULT hRes = AssocQueryStringW(flags,ASSOCSTR_COMMAND,pszExt,NULL,szBuf,&dwLen);
			if (hRes == S_OK)
				ptrCmdLine = CopyCmdLine(szBuf);
			else if (hRes == E_POINTER)
			{
				if (!ptrCmdLine.allocate((dwLen+1)*sizeof(wchar_t)))
					OMEGA_THROW(ERROR_OUTOFMEMORY);

				hRes = AssocQueryStringW(flags,ASSOCSTR_COMMAND,pszExt,NULL,ptrCmdLine,&dwLen);
			}

			if (hRes == S_OK)
			{
				LPVOID lpBuffer = 0;
				if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
								   ptrCmdLine,0,0,(LPWSTR)&lpBuffer,0,(va_list*)&pszFile))
				{
					ptrCmdLine = CopyCmdLine((LPWSTR)lpBuffer);
					LocalFree(lpBuffer);
				}
			}
		}
		
		return ptrCmdLine;
	}
}

bool User::Process::is_relative_path(const wchar_t* pszPath)
{
	return (PathIsRelativeW(pszPath) != FALSE);
}

User::Process* User::Process::exec(const wchar_t* pszExeName)
{
	// Do a ShellExecute style lookup for the actual thing to call..
	OOBase::SmartPtr<UserProcessWin32> ptrProcess = new (std::nothrow) UserProcessWin32();
	if (!ptrProcess)
		OMEGA_THROW(ERROR_OUTOFMEMORY);

	ptrProcess->exec(ShellParse(pszExeName));
	return ptrProcess.detach();
}

void UserProcessWin32::exec(OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> ptrCmdLine)
{
	DWORD dwFlags = DETACHED_PROCESS;

	OOBase::Win32::SmartHandle hDebugEvent;
	if (User::is_debug())
	{
		hDebugEvent = CreateEventW(NULL,FALSE,FALSE,L"Local\\OOCORE_DEBUG_MUTEX");

		dwFlags = CREATE_NEW_CONSOLE;
	}

	STARTUPINFOW si = {0};
	si.cb = sizeof(STARTUPINFOW);

	// Spawn the process
	PROCESS_INFORMATION pi = {0};
	if (!CreateProcessW(NULL,ptrCmdLine,NULL,NULL,FALSE,dwFlags,NULL,NULL,&si,&pi))
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

bool UserProcessWin32::wait_for_exit(const OOBase::timeval_t* wait, int& exit_code)
{
	if (!m_hProcess.is_valid())
		return true;

	DWORD dwWait = WaitForSingleObject(m_hProcess,(wait ? wait->msec() : INFINITE));
	if (dwWait == WAIT_OBJECT_0)
	{
		DWORD dwCode;
		if (GetExitCodeProcess(m_hProcess,&dwCode))
			exit_code = dwCode;
		else
			exit_code = -1;

		return true;
	}
	else if (dwWait != WAIT_TIMEOUT)
		OOBase_CallCriticalFailure(GetLastError());

	return false;
}

#endif // _WIN32
