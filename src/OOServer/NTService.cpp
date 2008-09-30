///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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

/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "./OOServer_Root.h"

#if defined(OMEGA_WIN32)

#include "./NTService.h"
#include "./RootManager.h"

ACE_NT_SERVICE_DEFINE(OOServer,Root::NTService,NTSERVICE_DESC);

Root::NTService::NTService() : 
	ACE_NT_Service(NTSERVICE_NAME,NTSERVICE_DESC),
	m_svc_thread(-1)
{
}

Root::NTService::~NTService()
{
}

bool Root::NTService::open()
{	
	// Do the ServiceMain in a separate thread
	NTSERVICE::instance()->m_svc_thread = ACE_Thread_Manager::instance()->spawn(NTService::start_service);
	if (NTSERVICE::instance()->m_svc_thread == -1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("Error spawning service thread")),false);
	
	return true;
}

void Root::NTService::stop()
{
	if (NTSERVICE::instance()->m_svc_thread != -1)
	{
		// Wait for any other threads that have been created...
		ACE_Thread_Manager::instance()->wait_grp(NTSERVICE::instance()->m_svc_thread);
	}
}

bool Root::NTService::install()
{
	// Stop the service if it is running
	NTSERVICE::instance()->stop_svc();

	// Remove the service config first, this allows us to alter the config
	NTSERVICE::instance()->remove();

	if (NTSERVICE::instance()->insert(ACE_TEXT("--service")) != 0 && GetLastError() != ERROR_SERVICE_MARKED_FOR_DELETE)
	{
		if (errno > 0 && errno <= 42)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Service install failed: %m")),false);
		else
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Service install failed: error %#x"),ACE_OS::last_error()),false);
	}

	NTSERVICE::instance()->description(NTSERVICE_LONGDESC);

	return true;
}

bool Root::NTService::uninstall()
{
	// Stop the service if it is running
	NTSERVICE::instance()->stop_svc();
	
	// Uninstall the service
	if (NTSERVICE::instance()->remove() != 0)
	{
		if (errno > 0 && errno <= 42)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Service uninstall failed: %m")),false);
		else
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Service uninstall failed: error %#x"),ACE_OS::last_error()),false);
	}

	return true;
}

int Root::NTService::insert(const ACE_TCHAR *cmd_line,
							DWORD start_type,
							DWORD error_control,
							const ACE_TCHAR *group_name,
							LPDWORD tag_id,
							const ACE_TCHAR *dependencies,
							const ACE_TCHAR *account_name,
							const ACE_TCHAR *password)
{
	ACE_TCHAR this_exe[PATH_MAX + 2];

	if (ACE_TEXT_GetModuleFileName(0,this_exe+1,PATH_MAX) == 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: GetModuleFilename failed: %#x\n"),GetLastError()),-1);
		
	// Make sure that this_exe is quoted
	this_exe[0] = ACE_TEXT('\"');
	ACE_OS::strcat(this_exe,ACE_TEXT("\""));
	
	ACE_TString exe_path(this_exe);
	exe_path += ACE_TEXT(" ");
	exe_path += cmd_line;

	return ACE_NT_Service::insert(start_type,error_control,exe_path.c_str(),group_name,tag_id,dependencies,account_name,password);
}

int Root::NTService::description(const ACE_TCHAR *desc)
{
	SC_HANDLE svc = this->svc_sc_handle ();
	if (svc == 0)
		return -1;

#if defined(ACE_USES_WCHAR)
	SERVICE_DESCRIPTIONW sdesc;
	sdesc.lpDescription = const_cast<LPWSTR>(desc);
	return ChangeServiceConfig2W(svc,(DWORD)SERVICE_CONFIG_DESCRIPTION,(LPVOID)&sdesc) ? 0 : -1;
#else
	SERVICE_DESCRIPTIONA sdesc;
	sdesc.lpDescription = const_cast<LPSTR>(desc);
	return ChangeServiceConfig2A(svc,(DWORD)SERVICE_CONFIG_DESCRIPTION,(LPVOID)&sdesc) ? 0 : -1;
#endif
}

BOOL Root::NTService::control_c(DWORD)
{
	// Just stop!
	Manager::end();

	return TRUE;
}

ACE_THR_FUNC_RETURN Root::NTService::start_service(void*)
{
	// This blocks running svc
	ACE_NT_SERVICE_RUN(OOServer,NTSERVICE::instance(),ret);
	if (!ret)
		SetConsoleCtrlHandler(control_c,TRUE);
		
	return 0;
}

int Root::NTService::svc(void)
{
	report_status(SERVICE_RUNNING);

	// Just wait here until we are told to stop
	int ret = m_finished.wait();

	report_status(SERVICE_STOPPED);

	return ret;
}

void Root::NTService::pause_requested(DWORD)
{
	// We don't pause
	report_status(SERVICE_RUNNING);
}

void Root::NTService::continue_requested(DWORD)
{
	// We don't pause
	report_status(SERVICE_RUNNING);
}

void Root::NTService::stop_requested(DWORD)
{
	report_status(SERVICE_STOP_PENDING);

	Manager::end();

	// Tell the service thread to stop
	m_finished.signal();
}

int Root::NTService::fini()
{
	ACE_OS::cleanup_tss(0);
	return 0;
}

#endif // OMEGA_WIN32
