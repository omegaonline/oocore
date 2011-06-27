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
		IObject* pIPS = NULL;
		ptrOMSB->GetRemoteInstance(OOCore::OID_InterProcessService,Activation::Library | Activation::DontLaunch,OMEGA_GUIDOF(OOCore::IInterProcessService),pIPS);
		m_ptrSBIPS.Attach(static_cast<OOCore::IInterProcessService*>(pIPS));
	}

	if (ptrOMUser)
	{
		// Create a proxy to the server interface
		IObject* pIPS = NULL;
		ptrOMUser->GetRemoteInstance(OOCore::OID_InterProcessService,Activation::Library | Activation::DontLaunch,OMEGA_GUIDOF(OOCore::IInterProcessService),pIPS);
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

void User::InterProcessService::LaunchObjectApp(const guid_t& oid, const guid_t& iid, Activation::Flags_t flags, IObject*& pObject)
{
	// Forward to sandbox if required
	if (m_ptrSBIPS && (flags & 0xF) == Activation::Sandbox)
		return m_ptrSBIPS->LaunchObjectApp(oid,iid,flags,pObject);
		
	// Find the OID key...
	string_t strProcess;
	ObjectPtr<Omega::Registry::IKey> ptrKey(L"Local User/Objects/OIDs/" + oid.ToString());
	if (ptrKey->IsValue(L"Application"))
	{
		// Find the name of the executable to run...
		string_t strAppName = ptrKey->GetValue(L"Application").cast<string_t>();
		ptrKey = ObjectPtr<Omega::Registry::IKey>(L"Local User/Applications/" + strAppName + L"/Activation");
		strProcess = ptrKey->GetValue(L"Path").cast<string_t>();
		if (strProcess.IsEmpty() || User::Process::is_relative_path(strProcess.c_wstr()))
		{
			string_t strErr = L"Relative path \"{0}\" in application '{1}' activation registry value." % strProcess % strAppName;
			OMEGA_THROW(strErr.c_nstr());
		}
	}
	else if (ptrKey->IsValue(L"Library"))
	{
		string_t strLib = ptrKey->GetValue(L"Library").cast<string_t>();
		if (strLib.IsEmpty() || User::Process::is_relative_path(strLib.c_wstr()))
		{
			string_t strErr(L"Relative path \"{0}\" in object library '{1}' activation registry value." % strLib % oid);
			OMEGA_THROW(strErr.c_nstr());
		}
		
		void* ISSUE_8; // Surrogates here?!?
	}
	else
		throw Activation::IOidNotFoundException::Create(oid);
			
	// Build RegisterFlags
	Activation::RegisterFlags_t reg_mask = Activation::PublicScope;
		
	// Remote activation, add ExternalPublic flag
	if (flags & Activation::RemoteActivation)
		reg_mask |= Activation::ExternalPublic;

	// The timeout needs to be related to the request timeout...
#if defined(OMEGA_DEBUG)
	OOBase::timeval_t wait(60);
#else
	OOBase::timeval_t wait(5);
#endif

	ObjectPtr<Remoting::ICallContext> ptrCC;
	ptrCC.Attach(Remoting::GetCallContext());
	if (ptrCC)
	{
		uint32_t msecs = ptrCC->Timeout();
		if (msecs != (uint32_t)-1)
			wait = OOBase::timeval_t(msecs / 1000,(msecs % 1000) * 1000);
	}

	for (bool bStarted = false;!bStarted;)
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		OOBase::SmartPtr<User::Process> ptrProcess;
		if (m_mapInProgress.find(strProcess,ptrProcess) && !ptrProcess->running())
		{
			m_mapInProgress.erase(strProcess);
			ptrProcess = NULL;
		}

		if (!ptrProcess)
		{
			OOSvrBase::Logger::log(OOSvrBase::Logger::Debug,"Executing process %ls",strProcess.c_wstr());

			// Create a new process
			ptrProcess = User::Process::exec(strProcess.c_wstr());

			int err = m_mapInProgress.insert(strProcess,ptrProcess);
			if (err != 0)
				OMEGA_THROW(err);

			bStarted = true;
		}

		guard.release();

		// Wait for the process to start and register its parts...
		OOBase::Countdown timeout(&wait);
		while (wait != OOBase::timeval_t::Zero)
		{
			m_ptrROT->GetObject(oid,reg_mask,iid,pObject);
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

	throw Activation::IOidNotFoundException::Create(oid);
}

bool_t User::InterProcessService::HandleRequest(uint32_t timeout)
{
	OOBase::timeval_t wait(timeout/1000,(timeout % 1000) * 1000);

	int ret = m_pManager->pump_requests((timeout ? &wait : NULL),true);
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
