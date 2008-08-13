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

#ifndef OOSERVER_INTER_PROCESS_SERVICE_H_INCLUDED_
#define OOSERVER_INTER_PROCESS_SERVICE_H_INCLUDED_

#include "./UserROT.h"
#include "./UserRegistry.h"

#include "../OOCore/Server.h"

namespace User
{
	class InterProcessService :
		public OTL::ObjectBase,
		public Omega::System::IInterProcessService
	{
	public:
		void Init(OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOMSB, OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOMUser, Manager* pManager);

		BEGIN_INTERFACE_MAP(InterProcessService)
			INTERFACE_ENTRY(Omega::System::IInterProcessService)
		END_INTERFACE_MAP()

	private:
		ACE_Thread_Mutex                                      m_lock;
		OTL::ObjectPtr<Omega::System::IInterProcessService> m_ptrSBIPS;
		OTL::ObjectPtr<OTL::ObjectImpl<RunningObjectTable> >  m_ptrROT;
		OTL::ObjectPtr<Omega::Registry::IKey>                 m_ptrReg;
		Manager*                                              m_pManager;

		std::map<Omega::string_t,ACE_Refcounted_Auto_Ptr<ACE_Process,ACE_Null_Mutex> > m_mapInProgress;

	// System::IInterProcessService members
	public:
		Omega::Registry::IKey* GetRegistry();
		Omega::Activation::IRunningObjectTable* GetRunningObjectTable();
		void GetObject(const Omega::string_t& strProcess, Omega::bool_t bPublic, const Omega::guid_t& oid, const Omega::guid_t& iid, Omega::IObject*& pObject);
		Omega::IO::IStream* OpenStream(const Omega::string_t& strEndpoint, Omega::IO::IAsyncStreamNotify* pNotify);
		Omega::bool_t HandleRequest(Omega::uint32_t timeout);
		Omega::Remoting::IChannel* OpenRemoteChannel(const Omega::string_t& strEndpoint);
		Omega::Remoting::IChannelSink* OpenServerSink(const Omega::guid_t& message_oid, Omega::Remoting::IChannelSink* pSink);
	};
}

#endif // OOSERVER_INTER_PROCESS_SERVICE_H_INCLUDED_
