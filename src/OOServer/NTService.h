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

#ifndef OOSERVER_NT_SERVICE_H_INCLUDED_
#define OOSERVER_NT_SERVICE_H_INCLUDED_

#if defined(ACE_WIN32)

#define NTSERVICE_NAME		ACE_TEXT("OOServer")
#define NTSERVICE_DESC		ACE_TEXT("Omega Online Network Gateway")
#define NTSERVICE_LONGDESC	ACE_TEXT("Manages the peer connections for the Omega Online network")

namespace Root
{
	class NTService : public ACE_NT_Service
	{
	public:
		NTService();
		virtual ~NTService();

		static bool open();
		static void stop();
		static bool install();
		static bool uninstall();
		
	private:
		typedef ACE_Singleton<NTService, ACE_Thread_Mutex> NTSERVICE;

		static ACE_THR_FUNC_RETURN start_service(void*);
		static BOOL WINAPI control_c(DWORD);
		
		int description(const ACE_TCHAR *desc);
		int insert(const ACE_TCHAR *cmd_line = 0,
		           DWORD start_type = SERVICE_DEMAND_START,
		           DWORD error_control = SERVICE_ERROR_IGNORE,
		           const ACE_TCHAR *group_name = 0,
		           LPDWORD tag_id = 0,
		           const ACE_TCHAR *dependencies = 0,
		           const ACE_TCHAR *account_name = 0,
		           const ACE_TCHAR *password = 0);

		int svc();
		int fini();
		void stop_requested(DWORD control_code);
		void pause_requested(DWORD control_code);
		void continue_requested(DWORD control_code);
		
		ACE_Event m_finished;
		int       m_svc_thread;
	};
}

#endif // ACE_WIN32

#endif // OOSERVER_NT_SERVICE_H_INCLUDED_
