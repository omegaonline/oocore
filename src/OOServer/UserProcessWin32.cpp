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

		void exec(OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> ptrCmdLine, OOBase::RefPtr<OOBase::Buffer>& env_block);

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

	bool env_sort(const OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator>& s1, const OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator>& s2)
	{
		return (_wcsicmp(s1,s2) < 0);
	}

	OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> to_wchar_t(const Omega::string_t& str)
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
			OMEGA_THROW(ERROR_OUTOFMEMORY);
		
		MultiByteToWideChar(CP_UTF8,0,str.c_str(),-1,wsz,len);
		wsz[len] = L'\0';
		return wsz;
	}
}

bool User::Process::is_relative_path(const Omega::string_t& strPath)
{
	return (PathIsRelativeW(to_wchar_t(strPath)) != FALSE);
}

User::Process* User::Process::exec(const Omega::string_t& strExeName, OOBase::Set<Omega::string_t,OOBase::LocalAllocator>& env)
{
	// Copy and widen to UNICODE
	OOBase::Set<OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator>,OOBase::LocalAllocator> wenv;
	for (size_t i=0;i<env.size();++i)
		wenv.insert(to_wchar_t(*env.at(i)));

	// Sort environment block - UNICODE, no-locale, case-insensitive (from MSDN)
	wenv.sort(&env_sort);

	// Build environment block
	OOBase::RefPtr<OOBase::Buffer> env_block = new (std::nothrow) OOBase::Buffer();
	if (!env_block)
		OMEGA_THROW(ERROR_OUTOFMEMORY);

	for (size_t i=0;i<wenv.size();++i)
	{
		wchar_t* e = *wenv.at(i);
		if (e)
		{
			size_t len = (wcslen(e) + 1)*sizeof(wchar_t);
			int err = env_block->space(len);
			if (err != 0)
				OMEGA_THROW(err);

			memcpy(env_block->wr_ptr(),e,len);
			env_block->wr_ptr()[len-1] = L'\0';
			env_block->wr_ptr(len);
		}
	}

	int err = env_block->space(4);
	if (err != 0)
		OMEGA_THROW(err);

	memset(env_block->wr_ptr(),0,4);
	env_block->wr_ptr(4);

	OOBase::SmartPtr<UserProcessWin32> ptrProcess = new (std::nothrow) UserProcessWin32();
	if (!ptrProcess)
		OMEGA_THROW(ERROR_OUTOFMEMORY);

	// Do a ShellExecute style lookup for the actual thing to call..
	ptrProcess->exec(ShellParse(to_wchar_t(strExeName)),env_block);
	return ptrProcess.detach();
}

void UserProcessWin32::exec(OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> ptrCmdLine, OOBase::RefPtr<OOBase::Buffer>& env_block)
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
	if (!CreateProcessW(NULL,ptrCmdLine,NULL,NULL,FALSE,dwFlags,(void*)(env_block->rd_ptr()),NULL,&si,&pi))
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

#endif // _WIN32
