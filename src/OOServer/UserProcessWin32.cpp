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

#if defined(OMEGA_DEBUG)
void AttachDebugger(DWORD pid);
#endif

namespace
{
	class UserProcessWin32 : public User::Process
	{
	public:
		virtual ~UserProcessWin32() {}

		virtual bool running();
		virtual bool wait_for_exit(const OOBase::timeval_t* wait, int* exit_code);

		void exec(const std::wstring& strExeName);

	private:
		OOBase::Win32::SmartHandle m_hProcess;
	};

	static std::wstring ShellParse(const wchar_t* pszFile)
	{
		std::wstring strRet = pszFile;

		const wchar_t* pszExt = PathFindExtensionW(pszFile);
		if (pszExt && wcsicmp(pszExt,L".exe")!=0)
		{
			DWORD dwLen = 1024;
			wchar_t szBuf[1024];
			ASSOCF flags = (ASSOCF)(ASSOCF_NOTRUNCATE | ASSOCF_REMAPRUNDLL);
			HRESULT hRes = AssocQueryStringW(flags,ASSOCSTR_COMMAND,pszExt,NULL,szBuf,&dwLen);
			if (hRes == S_OK)
				strRet = szBuf;
			else if (hRes == E_POINTER)
			{
				OOBase::SmartPtr<wchar_t,OOBase::ArrayDestructor<wchar_t> > pszBuf;
				OOBASE_NEW(pszBuf,wchar_t[dwLen+1]);
				if (pszBuf)
				{
					hRes = AssocQueryStringW(flags,ASSOCSTR_COMMAND,pszExt,NULL,pszBuf.value(),&dwLen);
					if (hRes==S_OK)
						strRet = pszBuf.value();
				}
			}

			if (hRes == S_OK)
			{
				LPVOID lpBuffer = 0;
				if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
					strRet.c_str(),0,0,(LPWSTR)&lpBuffer,0,(va_list*)&pszFile))
				{
					strRet = (LPWSTR)lpBuffer;
					LocalFree(lpBuffer);
				}
			}
		}

		return strRet;
	}
}

User::Process* User::Process::exec(const std::wstring& strExeName)
{
	// Do a ShellExecute style lookup for the actual thing to call..
	std::wstring strActualName = ShellParse(strExeName.c_str());

	OOBase::SmartPtr<UserProcessWin32> ptrProcess;
	OOBASE_NEW(ptrProcess,UserProcessWin32());
	if (!ptrProcess)
		OMEGA_THROW(ERROR_OUTOFMEMORY);
		
	ptrProcess->exec(strExeName);
	return ptrProcess.detach();
}

void UserProcessWin32::exec(const std::wstring& strExeName)
{
#if defined(OMEGA_DEBUG)
	OOBase::Win32::SmartHandle hDebugEvent;
	if (IsDebuggerPresent())
		hDebugEvent = CreateEventW(NULL,FALSE,FALSE,L"Local\\OOCORE_DEBUG_MUTEX");

#endif // OMEGA_DEBUG

	STARTUPINFOW si = {0};
	si.cb = sizeof(STARTUPINFOW);

	// Spawn the process
	PROCESS_INFORMATION pi = {0};
	if (!CreateProcessW(NULL,const_cast<LPWSTR>(strExeName.c_str()),NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
		OMEGA_THROW(GetLastError());

#if defined(OMEGA_DEBUG)
	if (hDebugEvent)
	{
		AttachDebugger(pi.dwProcessId);
		SetEvent(hDebugEvent);
	}
#endif

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

bool UserProcessWin32::wait_for_exit(const OOBase::timeval_t* wait, int* exit_code)
{
	if (!m_hProcess.is_valid())
		return true;

	DWORD dwWait = WaitForSingleObject(m_hProcess,(wait ? wait->msec() : INFINITE));
	if (dwWait == WAIT_OBJECT_0)
	{
		DWORD dwCode;
		if (GetExitCodeProcess(m_hProcess,&dwCode))
			*exit_code = dwCode;
		
		return true;
	}
	else if (dwWait != WAIT_TIMEOUT)
		OOBase_CallCriticalFailure(GetLastError());

	return false;
}

#endif // _WIN32