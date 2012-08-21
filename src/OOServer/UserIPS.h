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

#include "UserROT.h"
#include "UserProcess.h"

namespace User
{
	class InterProcessService :
			public OTL::ObjectBase,
			public OOCore::IInterProcessService
	{
	public:
		void init(Omega::Remoting::IObjectManager* pOMSB);

		BEGIN_INTERFACE_MAP(InterProcessService)
			INTERFACE_ENTRY(OOCore::IInterProcessService)
		END_INTERFACE_MAP()

	private:
		OOBase::Mutex                                         m_lock;
		OTL::ObjectPtr<OOCore::IInterProcessService>          m_ptrSBIPS;
		OTL::ObjectPtr<OTL::ObjectImpl<RunningObjectTable> >  m_ptrROT;
		OTL::ObjectPtr<Omega::Registry::IKey>                 m_ptrReg;

		OOBase::Table<Omega::string_t,OOBase::SmartPtr<User::Process> > m_mapInProgress;

		Omega::string_t GetSurrogateProcess(const Omega::guid_t& oid);

	// OOCore::IInterProcessService members
	public:
		Omega::Registry::IKey* GetRegistry();
		Omega::Activation::IRunningObjectTable* GetRunningObjectTable();
		void LaunchObjectApp(const Omega::guid_t& oid, const Omega::guid_t& iid, Omega::Activation::Flags_t flags, Omega::IObject*& pObject);
		Omega::bool_t HandleRequest(Omega::uint32_t millisecs);
		Omega::Remoting::IChannel* OpenRemoteChannel(const Omega::string_t& strEndpoint);
		Omega::Remoting::IChannelSink* OpenServerSink(const Omega::guid_t& message_oid, Omega::Remoting::IChannelSink* pSink);
	};
}

#endif // OOSERVER_INTER_PROCESS_SERVICE_H_INCLUDED_
