///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOSvrBase, the Omega Online Base library.
//
// OOSvrBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOSvrBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "Logger.h"
#include "Singleton.h"

#if defined(_WIN32)

#include <io.h>
#include <fcntl.h>

namespace
{
	class LoggerImpl
	{
	public:
		LoggerImpl();
		~LoggerImpl();

		void open(const char* name);
		void log(OOSvrBase::Logger::Priority priority, const char* fmt, va_list args);

	private:
		HANDLE        m_hLog;
		FILE*         m_stderr;
		OOBase::Mutex m_lock;
	};
}

LoggerImpl::LoggerImpl() : 
	m_hLog(NULL),
	m_stderr(NULL)
{
	// Make sure we can printf to the attached console
	HANDLE hStdErr = GetStdHandle(STD_ERROR_HANDLE);
	if (!hStdErr)
	{
		if (AttachConsole(ATTACH_PARENT_PROCESS))
			hStdErr = GetStdHandle(STD_ERROR_HANDLE);
	}

	if (hStdErr)
	{
		int fd = _open_osfhandle((intptr_t)hStdErr,_O_TEXT);
		if (fd != -1)
		{
			m_stderr = _fdopen(fd,"w");
		}
	}
}

LoggerImpl::~LoggerImpl()
{
	if (m_hLog)
		DeregisterEventSource(m_hLog);

	if (m_stderr)
		fclose(m_stderr);
}

void LoggerImpl::open(const char* name)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	if (m_hLog)
	{
		DeregisterEventSource(m_hLog);
		m_hLog = NULL;
	}

	wchar_t szPath[MAX_PATH];
	if (!GetModuleFileNameW(NULL,szPath,MAX_PATH))
		return;

	// Create the relevant registry keys if they don't already exist
	std::string strName = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
	strName += name;
		
	HKEY hk;
	DWORD dwDisp;
	if (RegCreateKeyExA(HKEY_LOCAL_MACHINE,strName.c_str(),0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hk,&dwDisp) == ERROR_SUCCESS) 
	{
		RegSetValueExW(hk,L"EventMessageFile",0,REG_EXPAND_SZ,(LPBYTE)szPath,(DWORD)wcslen(szPath)+1);

		DWORD dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE; 
 		RegSetValueExW(hk,L"TypesSupported",0,REG_DWORD,(LPBYTE)&dwData,sizeof(DWORD));

		RegCloseKey(hk);
	}

	// Get our event source
	m_hLog = RegisterEventSourceA(NULL,name);
}

void LoggerImpl::log(OOSvrBase::Logger::Priority priority, const char* fmt, va_list args)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	char szBuf[4096] = {0};
	vsnprintf_s(szBuf,sizeof(szBuf),fmt,args);
	szBuf[sizeof(szBuf)-1] = '\0';

	WORD wType = 0;
	switch (priority)
	{
	case OOSvrBase::Logger::Error:
		wType = EVENTLOG_ERROR_TYPE;
		break;

	case OOSvrBase::Logger::Warning:
		wType = EVENTLOG_WARNING_TYPE;
		break;

	case OOSvrBase::Logger::Information:
		wType = EVENTLOG_INFORMATION_TYPE;
		break;

	default:
		break;
	}

#if !defined(OMEGA_DEBUG)
	const char* arrBufs[2] = { szBuf, 0 };

	if (priority != OOSvrBase::Logger::Debug)
		ReportEventA(m_hLog,wType,0,0,NULL,1,0,arrBufs,NULL);
#endif

	OutputDebugStringA(szBuf);
	OutputDebugStringA("\n");
	
	if (m_stderr)
	{
		switch (priority)
		{
		case OOSvrBase::Logger::Error:
			fputs("Error: ",m_stderr);
			break;

		case OOSvrBase::Logger::Warning:
			fputs("Warning: ",m_stderr);
			break;

		default:
			break;
		}
		fputs(szBuf,m_stderr);
		fputs("\n",m_stderr);
		fflush(m_stderr);
	}
}

#elif defined(HAVE_ASL_H)
#include <asl.h>

#elif defined(HAVE_SYSLOG_H)
#include <syslog.h>

#else

#endif

namespace
{
	typedef OOBase::Singleton<LoggerImpl> LOGGER;
}

void OOSvrBase::Logger::open(const char* name)
{
	LOGGER::instance()->open(name);
}

void OOSvrBase::Logger::log(Priority priority, const char* fmt, ...)
{
	va_list args;
	va_start(args,fmt);

	LOGGER::instance()->log(priority,fmt,args);

	va_end(args);
}

std::string OOSvrBase::Logger::strerror(int err)
{
#if defined(_WIN32)
	return OOBase::Win32::FormatMessage(err);
#else
#error Fix me!
#endif
}

void OOSvrBase::Logger::filenum_t::log(const char* fmt, ...)
{
	va_list args;
	va_start(args,fmt);

	std::stringstream out;
	out << m_pszFilename << "(" << m_nLine << "): " << fmt;
	
	LOGGER::instance()->log(m_priority,out.str().c_str(),args);

	va_end(args);
}
