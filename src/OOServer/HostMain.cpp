///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2012 Rick Taylor
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

#include "OOServer_Host.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <sys/stat.h>
#endif

#if defined(_MSC_VER)
// Shutup VS leak
extern "C" int _setenvp() { return 0; }
#endif

#if defined(_WIN32)
#include <ShellAPI.h>
#endif

namespace
{
	int Help()
	{
		OOBase::stdout_write(APPNAME " - The Omega Online application host process.\n\n"
			"Please consult the documentation at http://www.omegaonline.org.uk for further information.\n\n");

		return EXIT_SUCCESS;
	}

	int Version()
	{
		OOBase::stdout_write(APPNAME " version " OOCORE_VERSION);

	#if !defined(NDEBUG)
		OOBase::stdout_write(" (Debug build)");
	#endif
		OOBase::stdout_write("\n\tCompiler: " OMEGA_COMPILER_STRING "\n\n");

		return EXIT_SUCCESS;
	}

	bool CriticalFailure(const char* msg)
	{
		OOBase::Logger::log(OOBase::Logger::Error,"%s",msg);

		if (Host::is_debug())
		{
			// Give us a chance to read the errors!
			OOBase::Thread::sleep(15000);
		}
		return true;
	}

	static bool s_is_debug = false;
}

bool Host::is_debug()
{
	return s_is_debug;
}

int main(int argc, char* argv[])
{
	// Start the logger - use OOServer again...
	OOBase::Logger::open("OOServer",__FILE__);

	// Set critical failure handler
	OOBase::SetCriticalFailure(&CriticalFailure);

	// Set up the command line args
	OOBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
	cmd_args.add_option("version",'v');
	cmd_args.add_option("debug");
	cmd_args.add_option("single");
	cmd_args.add_option("multiple");
	cmd_args.add_option("service");

#if defined(_WIN32)
	cmd_args.add_option("shellex");
#endif

	// Parse command line
	OOBase::CmdArgs::results_t args;
	int err = cmd_args.parse(argc,argv,args);
	if (err	!= 0)
	{
		OOBase::String strErr;
		if (args.find("missing",strErr))
			OOBase::Logger::log(OOBase::Logger::Error,APPNAME " - Missing value for option %s",strErr.c_str());
		else if (args.find("unknown",strErr))
			OOBase::Logger::log(OOBase::Logger::Error,APPNAME " - Unknown option %s",strErr.c_str());
		else
			OOBase::Logger::log(OOBase::Logger::Error,APPNAME " - Failed to parse command line: %s",OOBase::system_error_text(err));

		return EXIT_FAILURE;
	}

	s_is_debug = args.exists("debug");

	if (args.exists("help"))
		return Help();

	if (args.exists("version"))
		return Version();

#if defined(_WIN32)
	// If this event exists, then we are being debugged
	{
		// Scope it...
		OOBase::Win32::SmartHandle hDebugEvent(OpenEventW(EVENT_ALL_ACCESS,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX"));
		if (hDebugEvent)
		{
			// Wait for a bit, letting the caller attach a debugger
			WaitForSingleObject(hDebugEvent,15000);
		}
	}

#elif defined(HAVE_UNISTD_H) && 0

	if (s_is_debug)
	{
		printf("Attach a debugger to " APPNAME " process %u, and hit ENTER...\n",getpid());
		char buf[256];
		read(STDIN_FILENO,buf,sizeof(buf));
	}

#endif

	if (args.exists("multiple"))
		return Host::MultipleSurrogate();
	else if (args.exists("single"))
		return Host::SingleSurrogate();
	else if (args.exists("service"))
		return Host::ServiceStart();
#if defined(_WIN32)
	else if (args.exists("shellex"))
		return Host::ShellEx(args);
#endif

	// Oops...
	OOBase::Logger::log(OOBase::Logger::Error,APPNAME " - Invalid or missing arguments.");
	return EXIT_FAILURE;
}

#if defined(_WIN32)

#if !defined(SEE_MASK_NOASYNC)
#define SEE_MASK_NOASYNC SEE_MASK_FLAG_DDEWAIT
#endif

namespace
{
	template <typename T>
	OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> to_wchar_t(const T& str)
	{
		OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> wsz
		int len = MultiByteToWideChar(CP_UTF8,0,str.c_str(),-1,NULL,0);
		if (len == 0)
		{
			DWORD dwErr = GetLastError();
			if (dwErr != ERROR_INSUFFICIENT_BUFFER)
				LOG_ERROR_RETURN(("Failed to convert UTF8 to wchar_t: %s",OOBase::system_error_text(dwErr)),wsz);
		}

		wsz.allocate((len+1) * sizeof(wchar_t));
		if (!wsz)
			LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text()),wsz);
		
		MultiByteToWideChar(CP_UTF8,0,str.c_str(),-1,wsz,len);
		wsz[len] = L'\0';
		return wsz;
	}
}

int Host::ShellEx(const OOBase::CmdArgs::results_t& args)
{
	OOBase::String strAppName;
	if (!args.find("@0",strAppName))
		LOG_ERROR_RETURN(("No arguments passed with --shellex"),EXIT_FAILURE);

	OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> wszAppName = to_wchar_t(strAppName);
	if (!wszAppName)
		return EXIT_FAILURE;

	OOBase::LocalString strCmdLine;
	for (size_t i = 1;;++i)
	{
		OOBase::LocalString strId;
		int err = strId.printf("@%u",i);
		if (err)
			LOG_ERROR_RETURN(("Failed to format string: %s",OOBase::system_error_text(err)),EXIT_FAILURE);

		OOBase::String strArg;
		if (!args.find(strId.c_str(),strArg))
			break;

		err = strCmdLine.append(strArg.c_str());
		if (!err && i != 0)
			err = strCmdLine.append(" ");
		if (err)
			LOG_ERROR_RETURN(("Failed to append string: %s",OOBase::system_error_text(err)),EXIT_FAILURE);
	}

	OOBase::SmartPtr<wchar_t,OOBase::LocalAllocator> wszCmdLine;
	if (!strCmdLine.empty())
	{
		wszCmdLine = to_wchar_t(strCmdLine);
		if (!wszCmdLine)
			return EXIT_FAILURE;
	}
	
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr))
		LOG_ERROR_RETURN(("CoInitializeEx failed: %s",OOBase::system_error_text()),EXIT_FAILURE);

	SHELLEXECUTEINFOW sei = {0};
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE;
	sei.lpFile = wszAppName;
	sei.lpParameters = wszCmdLine;
	sei.nShow = SW_SHOWDEFAULT;

	if (!ShellExecuteExW(&sei))
		LOG_ERROR_RETURN(("ShellExecuteExW failed: %s",OOBase::system_error_text()),EXIT_FAILURE);

	if (sei.hProcess)
	{
		WaitForSingleObject(sei.hProcess,INFINITE);
		CloseHandle(sei.hProcess);
	}

	CoUninitialize();

	return EXIT_SUCCESS;
}
#endif
