///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
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

#if defined(ACE_WIN32)

#include "./NTService.h"
#include "./RootManager.h"

ACE_NT_SERVICE_DEFINE(OOServer,Root::NTService,NTSERVICE_DESC);

Root::NTService::NTService() : ACE_NT_Service(NTSERVICE_NAME,NTSERVICE_DESC)
{
}

Root::NTService::~NTService()
{
}

bool Root::NTService::open()
{	
    // Do the ServiceMain in a separate thread
	if (ACE_Thread_Manager::instance()->spawn(NTService::start_service) == -1)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"Error spawning service thread"),false);
	
	return true;
}

bool Root::NTService::install()
{
	// Stop the service if it is running
	NTSERVICE::instance()->stop_svc();

	// Remove the service config first, this allows us to alter the config
	NTSERVICE::instance()->remove();

	if (NTSERVICE::instance()->insert(L"--service") != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Service install"),false);

	NTSERVICE::instance()->description(NTSERVICE_LONGDESC);

	return true;
}

bool Root::NTService::uninstall()
{
	// Stop the service if it is running
	NTSERVICE::instance()->stop_svc();
	
	// Uninstall the service
	if (NTSERVICE::instance()->remove() != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Service uninstall"),false);

	return true;
}

int Root::NTService::insert(const wchar_t *cmd_line,
							DWORD start_type,
							DWORD error_control,
							const wchar_t *group_name,
							LPDWORD tag_id,
							const wchar_t *dependencies,
							const wchar_t *account_name,
							const wchar_t *password)
{
	wchar_t this_exe[PATH_MAX + 2];

	if (GetModuleFileNameW(0,this_exe+1,PATH_MAX) == 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] GetModuleFilename failed: %#x\n",GetLastError()),-1);
		
	// Make sure that this_exe is quoted
	this_exe[0] = L'\"';
	ACE_OS::strcat(this_exe,L"\"");
	
	ACE_WString exe_path(this_exe);
	exe_path += L" ";
	exe_path += cmd_line;

	return ACE_NT_Service::insert(start_type,error_control,exe_path.c_str(),group_name,tag_id,dependencies,account_name,password);
}

int Root::NTService::description(const wchar_t *desc)
{
	SC_HANDLE svc = this->svc_sc_handle ();
	if (svc == 0)
		return -1;

	SERVICE_DESCRIPTIONW sdesc;
	sdesc.lpDescription = const_cast<LPWSTR>(desc);

	BOOL ok = ::ChangeServiceConfig2W(svc,
							(DWORD)SERVICE_CONFIG_DESCRIPTION,	// Change the description
							(LPVOID)&sdesc);

	return ok ? 0 : -1;
}

BOOL Root::NTService::control_c(DWORD)
{
	Root::NTService::NTSERVICE::instance()->stop_requested(0);

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

#endif // ACE_WIN32
