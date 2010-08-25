///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
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

#include "InterProcessService.h"
#include "UserManager.h"

using namespace Omega;
using namespace OTL;

void User::InterProcessService::Init(OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOMSB, OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOMUser, Manager* pManager)
{
	m_pManager = pManager;

	if (ptrOMSB)
	{
		// Create a proxy to the server interface
		IObject* pIPS = 0;
		ptrOMSB->GetRemoteInstance(OOCore::OID_InterProcessService,Activation::InProcess | Activation::DontLaunch,OMEGA_GUIDOF(OOCore::IInterProcessService),pIPS);
		m_ptrSBIPS.Attach(static_cast<OOCore::IInterProcessService*>(pIPS));
	}

	if (ptrOMUser)
	{
		// Create a proxy to the server interface
		IObject* pIPS = 0;
		ptrOMUser->GetRemoteInstance(OOCore::OID_InterProcessService,Activation::InProcess | Activation::DontLaunch,OMEGA_GUIDOF(OOCore::IInterProcessService),pIPS);
		ObjectPtr<OOCore::IInterProcessService> ptrIPS;
		ptrIPS.Attach(static_cast<OOCore::IInterProcessService*>(pIPS));

		// Get the running object table
		m_ptrReg.Attach(ptrIPS->GetRegistry());
	}
	else
	{
		// Create a local registry impl
		ObjectPtr<ObjectImpl<Registry::Key> > ptrKey = ObjectImpl<User::Registry::Key>::CreateInstancePtr();
		ptrKey->Init(m_pManager,string_t(),0,0);

		m_ptrReg = static_cast<Omega::Registry::IKey*>(ptrKey);
	}

	// Create the ROT
	m_ptrROT = ObjectImpl<User::RunningObjectTable>::CreateInstancePtr();
	try
	{
		m_ptrROT->Init(ptrOMSB);
	}
	catch (...)
	{
		m_ptrROT.Release();
		throw;
	}
}

Registry::IKey* User::InterProcessService::GetRegistry()
{
	return m_ptrReg.AddRef();
}

Activation::IRunningObjectTable* User::InterProcessService::GetRunningObjectTable()
{
	return m_ptrROT.AddRef();
}

void User::InterProcessService::LaunchObjectApp(const guid_t& oid, const guid_t& iid, IObject*& pObject)
{
	pObject = 0;

	// Find the OID key...
	ObjectPtr<Omega::Registry::IKey> ptrOidKey(L"/Local User/Objects/OIDs/" + oid.ToString());

	string_t strAppName = ptrOidKey->GetValue(L"Application").cast<string_t>();

	// Find the name of the executable to run...
	ObjectPtr<Omega::Registry::IKey> ptrServer(L"/Local User/Applications/" + strAppName + L"/Activation");

	string_t strProcess = ptrServer->GetValue(L"Path").cast<string_t>();

	// The timeout needs to be related to the request timeout...
#if defined(OMEGA_DEBUG)
	OOBase::timeval_t wait(60);
#else
	OOBase::timeval_t wait(15);
#endif

	ObjectPtr<Remoting::ICallContext> ptrCC;
	ptrCC.Attach(Remoting::GetCallContext());
	if (ptrCC)
	{
		uint32_t msecs = ptrCC->Timeout();
		if (msecs != (uint32_t)-1)
			wait = OOBase::timeval_t(msecs / 1000,(msecs % 1000) * 1000);
	}

	OOBase::SmartPtr<User::Process> ptrProcess;

	bool bStarted = false;
	do
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		std::map<string_t,OOBase::SmartPtr<User::Process> >::iterator i = m_mapInProgress.find(strProcess);
		if (i != m_mapInProgress.end())
		{
			ptrProcess = i->second;

			if (!ptrProcess->running())
			{
				m_mapInProgress.erase(strProcess);
				ptrProcess = 0;
			}
		}

		if (!ptrProcess)
		{
			OOSvrBase::Logger::log(OOSvrBase::Logger::Debug,"Executing process %ls",strProcess.c_str());

			// Create a new process
			ptrProcess = User::Process::exec(strProcess.c_str());

			m_mapInProgress.insert(std::map<string_t,OOBase::SmartPtr<User::Process> >::value_type(strProcess,ptrProcess));
			bStarted = true;
		}

		guard.release();

		// Wait for the process to start and register its parts...
		OOBase::Countdown timeout(&wait);
		while (wait != OOBase::timeval_t::Zero)
		{
			m_ptrROT->GetObject(oid,Activation::UserLocal | Activation::MachineLocal,iid,pObject);
			if (pObject)
			{
				// The process has started - remove it from the starting list
				guard.acquire();
				m_mapInProgress.erase(strProcess);
				guard.release();

				return;
			}

			// Check the process is still alive
			OOBase::timeval_t short_wait(0,100000);
			int ec = 0;
			if (ptrProcess->wait_for_exit(&short_wait,&ec))
				break;

			// Update our countdown
			timeout.update();
		}

		if (wait == OOBase::timeval_t::Zero)
			throw ITimeoutException::Create();

		// Remove from the map
		guard.acquire();
		m_mapInProgress.erase(strProcess);
		guard.release();

	}
	while (!bStarted);

	throw Activation::IOidNotFoundException::Create(oid);
}

bool_t User::InterProcessService::HandleRequest(uint32_t timeout)
{
	OOBase::timeval_t wait(timeout/1000,(timeout % 1000) * 1000);

	int ret = m_pManager->pump_requests((timeout ? &wait : 0),true);
	if (ret == -1)
		OMEGA_THROW("Request processing failed");
	else
		return (ret == 1);
}

Remoting::IChannel* User::InterProcessService::OpenRemoteChannel(const string_t& strEndpoint)
{
	return Manager::open_remote_channel(strEndpoint);
}

Remoting::IChannelSink* User::InterProcessService::OpenServerSink(const guid_t& message_oid, Remoting::IChannelSink* pSink)
{
	return Manager::open_server_sink(message_oid,pSink);
}

OMEGA_DEFINE_OID(OOCore,OID_InterProcessService,"{7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}");
