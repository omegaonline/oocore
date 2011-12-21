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

void User::InterProcessService::Init(Remoting::IObjectManager* pOMSB, Remoting::IObjectManager* pOMUser, Manager* pManager)
{
	m_pManager = pManager;

	if (pOMSB)
	{
		// Create a proxy to the server interface
		IObject* pIPS = NULL;
		pOMSB->GetRemoteInstance(OOCore::OID_InterProcessService,Activation::Library | Activation::DontLaunch,OMEGA_GUIDOF(OOCore::IInterProcessService),pIPS);
		m_ptrSBIPS = static_cast<OOCore::IInterProcessService*>(pIPS);
	}

	if (pOMUser)
	{
		// Create a proxy to the server interface
		IObject* pIPS = NULL;
		pOMUser->GetRemoteInstance(OOCore::OID_InterProcessService,Activation::Library | Activation::DontLaunch,OMEGA_GUIDOF(OOCore::IInterProcessService),pIPS);
		ObjectPtr<OOCore::IInterProcessService> ptrIPS = static_cast<OOCore::IInterProcessService*>(pIPS);

		// Get the running object table
		m_ptrReg = ptrIPS->GetRegistry();
	}
	else
	{
		// Create a local registry impl
		ObjectPtr<ObjectImpl<Registry::Key> > ptrKey = ObjectImpl<User::Registry::Key>::CreateInstance();
		ptrKey->Init(m_pManager,string_t(),0,0);
		m_ptrReg = ptrKey.AddRef();
	}

	// Create the ROT
	m_ptrROT = ObjectImpl<User::RunningObjectTable>::CreateInstance();
	try
	{
		m_ptrROT->Init(pOMSB);
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

void User::InterProcessService::LaunchObjectApp(const guid_t& oid, const guid_t& iid, Activation::Flags_t flags, Omega::uint32_t envc, const Omega::string_t* envp, IObject*& pObject)
{
	// Forward to sandbox if required
	if (m_ptrSBIPS && (flags & 0xF) == Activation::Sandbox)
		return m_ptrSBIPS->LaunchObjectApp(oid,iid,flags,envc,envp,pObject);

	// Find the OID key...
	string_t strProcess;
	ObjectPtr<Omega::Registry::IKey> ptrKey("Local User/Objects/OIDs/" + oid.ToString());
	if (ptrKey->IsValue(string_t::constant("Application")))
	{
		// Find the name of the executable to run...
		string_t strAppName = ptrKey->GetValue(string_t::constant("Application")).cast<string_t>();
		ptrKey = ObjectPtr<Omega::Registry::IKey>("Local User/Applications/" + strAppName + "/Activation");
		strProcess = ptrKey->GetValue(string_t::constant("Path")).cast<string_t>();
		if (strProcess.IsEmpty() || User::Process::is_relative_path(strProcess))
		{
			string_t strErr = string_t::constant("Relative path \"{0}\" in application '{1}' activation registry value.") % strProcess % strAppName;
			OMEGA_THROW(strErr.c_str());
		}
	}
	else if (ptrKey->IsValue(string_t::constant("Library")))
	{
		string_t strLib = ptrKey->GetValue(string_t::constant("Library")).cast<string_t>();
		if (strLib.IsEmpty() || User::Process::is_relative_path(strLib))
		{
			string_t strErr = string_t::constant("Relative path \"{0}\" in object library '{1}' activation registry value.") % strLib % oid;
			OMEGA_THROW(strErr.c_str());
		}
		
		void* ISSUE_8; // Surrogates here?!?

		throw Activation::IOidNotFoundException::Create(oid);
	}
	else
		throw Activation::IOidNotFoundException::Create(oid);

	// Build the environment block
	OOBase::Set<string_t,OOBase::LocalAllocator> setEnv;
	for (uint32_t i = 0; i < envc; ++i)
	{
		// Remove any unwanted entries
		void* TODO;

		int err = setEnv.insert(envp[i]);
		if (err != 0)
			OMEGA_THROW(err);
	}
			
	// Build RegisterFlags
	Activation::RegisterFlags_t reg_mask = Activation::PublicScope;
		
	// Remote activation, add ExternalPublic flag
	if (flags & Activation::RemoteActivation)
		reg_mask |= Activation::ExternalPublic;

	// The timeout needs to be related to the request timeout...
	OOBase::Countdown countdown(15,0);
	ObjectPtr<Remoting::ICallContext> ptrCC = Remoting::GetCallContext();
	if (ptrCC)
	{
		uint32_t msecs = ptrCC->Timeout();
		if (msecs != (uint32_t)-1)
			countdown = OOBase::Countdown(msecs / 1000,(msecs % 1000) * 1000);
	}

	for (bool bStarted = false;!bStarted;)
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock,false);
		if (!guard.acquire(countdown))
			throw ITimeoutException::Create();

		OOBase::SmartPtr<User::Process> ptrProcess;
		if (m_mapInProgress.find(strProcess,ptrProcess) && !ptrProcess->running())
		{
			m_mapInProgress.remove(strProcess);
			ptrProcess = NULL;
		}

		if (!ptrProcess)
		{
			OOBase::Logger::log(OOBase::Logger::Debug,"Executing process %s",strProcess.c_str());

			// Create a new process
			ptrProcess = User::Process::exec(strProcess,setEnv);

			int err = m_mapInProgress.insert(strProcess,ptrProcess);
			if (err != 0)
				OMEGA_THROW(err);

			bStarted = true;
		}

		guard.release();

		// Wait for the process to start and register its parts...
		
		while (!countdown.has_ended())
		{
			m_ptrROT->GetObject(oid,reg_mask,iid,pObject);
			if (pObject)
			{
				// The process has started - remove it from the starting list
				guard.acquire();
				m_mapInProgress.remove(strProcess);
				guard.release();
				return;
			}

			// Check the process is still alive
			OOBase::timeval_t short_wait(0,100000);
			int ec = 0;
			if (ptrProcess->wait_for_exit(&short_wait,ec))
				break;
		}

		// Remove from the map
		guard.acquire();
		m_mapInProgress.remove(strProcess);
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
