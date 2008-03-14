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

#ifndef OOSERVER_INTER_PROCESS_SERVICE_H_INCLUDED_
#define OOSERVER_INTER_PROCESS_SERVICE_H_INCLUDED_

#include "./UserROT.h"
#include "./UserRegistry.h"

namespace User
{
	void ExecProcess(ACE_Process& process, const Omega::string_t& strExeName);
	ACE_WString ShellParse(const wchar_t* pszFile);

	class InterProcessService :
		public OTL::ObjectBase,
		public Omega::Remoting::IInterProcessService
	{
	public:
		void Init(OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOMSB, OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOMUser, Manager* pManager)
		{
			m_ptrOMSB = ptrOMSB;
			m_ptrOMUser = ptrOMUser;
			m_pManager = pManager;
		}

		BEGIN_INTERFACE_MAP(InterProcessService)
			INTERFACE_ENTRY(Omega::Remoting::IInterProcessService)
		END_INTERFACE_MAP()

	private:
		ACE_Thread_Mutex                                     m_lock;
		OTL::ObjectPtr<Omega::Remoting::IObjectManager>      m_ptrOMSB;
		OTL::ObjectPtr<Omega::Remoting::IObjectManager>      m_ptrOMUser;
		OTL::ObjectPtr<OTL::ObjectImpl<RunningObjectTable> > m_ptrROT;
		OTL::ObjectPtr<Omega::Registry::IRegistryKey>        m_ptrReg;
		Manager*                                             m_pManager;

	// Remoting::IInterProcessService members
	public:
		Omega::Registry::IRegistryKey* GetRegistry();
		Omega::Activation::IRunningObjectTable* GetRunningObjectTable();
		void GetRegisteredObject(const Omega::guid_t& oid, Omega::Activation::Flags_t flags, const Omega::guid_t& iid, Omega::IObject*& pObject);
	};
}

#endif // OOSERVER_INTER_PROCESS_SERVICE_H_INCLUDED_
