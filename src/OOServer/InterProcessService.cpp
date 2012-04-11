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

void User::InterProcessService::init(Remoting::IObjectManager* pOMSB, Remoting::IObjectManager* pOMUser, Manager* pManager)
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
		ObjectPtr<ObjectImpl<Registry::RootKey> > ptrKey = ObjectImpl<User::Registry::RootKey>::CreateInstance();
		ptrKey->init(m_pManager,string_t(),0,0);
		m_ptrReg = ptrKey.AddRef();
	}

	// Create the ROT
	m_ptrROT = ObjectImpl<User::RunningObjectTable>::CreateInstance();
	try
	{
		m_ptrROT->init(pOMSB);
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

	// The timeout needs to be related to the request timeout...
	OOBase::Timeout timeout(15,0);
	ObjectPtr<Remoting::ICallContext> ptrCC = Remoting::GetCallContext();
	if (ptrCC)
	{
		uint32_t msecs = ptrCC->Timeout();
		if (msecs != 0xFFFFFFFF)
			timeout = OOBase::Timeout(msecs / 1000,(msecs % 1000) * 1000);
	}

	// Find the OID key...
	string_t strProcess;
	ObjectPtr<Omega::Registry::IKey> ptrLU = ObjectPtr<Omega::Registry::IOverlayKeyFactory>(Omega::Registry::OID_OverlayKeyFactory)->Overlay("Local User","All Users");
	ObjectPtr<Omega::Registry::IKey> ptrKey = ptrLU->OpenKey("Objects/OIDs/" + oid.ToString());

	if (ptrKey->IsValue(string_t::constant("Library")))
	{
		string_t strLib = ptrKey->GetValue(string_t::constant("Library")).cast<string_t>();
		if (strLib.IsEmpty() || User::Process::is_relative_path(strLib))
		{
			string_t strErr = string_t::constant("Relative path \"{0}\" in object library '{1}' activation registry value.") % strLib % oid;
			OMEGA_THROW(strErr.c_str());
		}

		void* ISSUE_8; // Surrogates here

		OMEGA_THROW("No surrogate support!");
	}
	else
	{
		// Find the name of the executable to run...
		string_t strAppName = ptrKey->GetValue(string_t::constant("Application")).cast<string_t>();
		ptrKey = ptrLU->OpenKey("Applications/" + strAppName + "/Activation");
		strProcess = ptrKey->GetValue(string_t::constant("Path")).cast<string_t>();
		if (strProcess.IsEmpty() || User::Process::is_relative_path(strProcess))
		{
			string_t strErr = string_t::constant("Relative path \"{0}\" in application '{1}' activation registry value.") % strProcess % strAppName;
			OMEGA_THROW(strErr.c_str());
		}
	}

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

	OOBase::Guard<OOBase::Mutex> guard(m_lock,false);
	if (!guard.acquire(timeout))
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
	}

	guard.release();

	// Wait for the process to start and register its parts...
	int exit_code = 0;
	while (!timeout.has_expired())
	{
		m_ptrROT->GetObject(oid,reg_mask,iid,pObject);
		if (pObject)
			break;

		// Check the process is still alive
		if (ptrProcess->wait_for_exit(OOBase::Timeout(0,1),exit_code))
			break;
	}

	// Remove from the map
	guard.acquire();
	m_mapInProgress.remove(strProcess);
	guard.release();

	if (!pObject)
	{
		if (timeout.has_expired())
			throw INotFoundException::Create(string_t::constant("The process {0} does not implement the object {1}") % strProcess % oid);
		else
			throw INotFoundException::Create(string_t::constant("The process {0} terminated unexpectedly with exit code {1}") % strProcess % exit_code);
	}
}

bool_t User::InterProcessService::HandleRequest(uint32_t millisecs)
{
	OOBase::Timeout timeout;
	if (millisecs != 0xFFFFFFFF)
		timeout = OOBase::Timeout(millisecs/1000,(millisecs % 1000) * 1000);

	int ret = m_pManager->pump_requests(timeout,true);
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
