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

#include "UserIPS.h"
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
		ptrKey->init(m_pManager,string_t::constant("/"),0,0);
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

string_t User::InterProcessService::GetSurrogateProcess(const guid_t& oid)
{
	string_t strProcess;
	m_pManager->get_root_config_arg("binary_path",strProcess);

#if defined(_WIN32)
	strProcess += "OOSvrHost.exe";
#else
	strProcess += "oosvrhost";
#endif

	LOG_DEBUG(("Running surrogate process: %s",strProcess.c_str()));

	if (User::is_debug())
		strProcess += " --debug";

	if (oid == OOCore::OID_Surrogate)
		strProcess += " --multiple";
	else if (oid == OOCore::OID_SingleSurrogate)
		strProcess += " --single";
	else
		strProcess.Clear();

	return strProcess;
}

void User::InterProcessService::LaunchObjectApp(const guid_t& oid, const guid_t& iid, Activation::Flags_t flags, IObject*& pObject)
{
	// Forward to sandbox if required
	if (m_ptrSBIPS && (flags & 0xF) >= Activation::Sandbox)
		return m_ptrSBIPS->LaunchObjectApp(oid,iid,flags,pObject);

	// The timeout needs to be related to the request timeout...
	OOBase::Timeout timeout(15,0);
	ObjectPtr<Remoting::ICallContext> ptrCC = Remoting::GetCallContext();
	if (ptrCC)
	{
		uint32_t msecs = ptrCC->Timeout();
		if (msecs != 0xFFFFFFFF)
			timeout = OOBase::Timeout(msecs / 1000,(msecs % 1000) * 1000);
	}

	// Use an infinite timeout if we are debugging
	if (User::is_debug())
		timeout = OOBase::Timeout();

	// Find the OID key...
	ObjectPtr<Omega::Registry::IKey> ptrLU = ObjectPtr<Omega::Registry::IOverlayKeyFactory>(Omega::Registry::OID_OverlayKeyFactory)->Overlay("Local User","All Users");
	ObjectPtr<Omega::Registry::IKey> ptrKey = ptrLU->OpenKey("Objects/OIDs");

	bool is_surrogate = false;
	string_t strProcess,strWorkingDir;
	if ((oid == OOCore::OID_Surrogate || oid == OOCore::OID_SingleSurrogate) && !ptrKey->IsKey(oid.ToString()))
	{
		strProcess = GetSurrogateProcess(oid);
		is_surrogate = !strProcess.IsEmpty();
	}

	OOBase::Table<OOBase::String,OOBase::String,OOBase::LocalAllocator> tabEnv;
	if (strProcess.IsEmpty())
	{
		// Find the name of the executable to run...
		ptrKey = ptrKey->OpenKey(oid.ToString());
		string_t strAppName = ptrKey->GetValue(string_t::constant("Application")).cast<string_t>();
		ptrKey = ptrLU->OpenKey("Applications/" + strAppName + "/Activation");
		strProcess = ptrKey->GetValue(string_t::constant("Path")).cast<string_t>();
		if (strProcess.IsEmpty() || User::Process::is_invalid_path(strProcess))
			throw IAccessDeniedException::Create(string_t::constant("Invalid path \"{0}\" in application activation registry value.") % strProcess);

		if (ptrKey->IsValue(string_t::constant("Directory")))
			strWorkingDir = ptrKey->GetValue(string_t::constant("Directory")).cast<string_t>();

		if (ptrKey->IsKey(string_t::constant("Environment")))
		{
			ptrKey = ptrKey->OpenKey(string_t::constant("Environment"));
			Omega::Registry::IKey::string_set_t setVals = ptrKey->EnumValues();
			for (Omega::Registry::IKey::string_set_t::const_iterator i=setVals.begin();i!=setVals.end();++i)
			{
				OOBase::String strKey,strVal;
				int err = strKey.assign(i->c_str(),i->Length());
				if (err)
					OMEGA_THROW(err);

				string_t val = ptrKey->GetValue(*i).cast<string_t>();
				err = strVal.assign(val.c_str(),val.Length());
				if (!err)
					err = tabEnv.insert(strKey,strVal);

				if (err)
					OMEGA_THROW(err);
			}
		}
	}

	// Get the environment settings
	OOBase::Table<OOBase::String,OOBase::String,OOBase::LocalAllocator> tabSysEnv;
	int err = OOBase::Environment::get_current(tabSysEnv);
	if (err)
		OMEGA_THROW(err);

	err = OOBase::Environment::substitute(tabEnv,tabSysEnv);
	if (err)
		OMEGA_THROW(err);

	// Check for remote activation
	bool remote_activation = false;
	if (flags & Activation::RemoteActivation)
		remote_activation = true;

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
		ptrProcess = User::Process::exec(strProcess,strWorkingDir,is_surrogate,tabEnv);

		int err = m_mapInProgress.insert(strProcess,ptrProcess);
		if (err != 0)
			OMEGA_THROW(err);
	}

	guard.release();

	// When we have notification support, use a notifier...
	void* TODO;

	// Wait for the process to start and register its parts...
	int exit_code = 0;
	for (unsigned int msecs = 1;!timeout.has_expired();msecs *= 2)
	{
		// Check the process is still alive
		if (ptrProcess->wait_for_exit(OOBase::Timeout(0,msecs),exit_code))
			break;

		m_ptrROT->GetObject(oid,iid,pObject,remote_activation);
		if (pObject)
			break;
	}

	// Remove from the map
	guard.acquire();
	m_mapInProgress.remove(strProcess);
	guard.release();

	if (!pObject)
	{
		if (timeout.has_expired())
		{
			OOBase::Logger::log(OOBase::Logger::Debug,"Given up waiting for process %s",strProcess.c_str());

			ptrProcess->kill();

			throw ITimeoutException::Create();
		}

		OOBase::Logger::log(OOBase::Logger::Debug,"Process %s exited with code %d",strProcess.c_str(),exit_code);

		throw INotFoundException::Create(string_t::constant("The process '{0}' terminated unexpectedly with exit code {1}") % strProcess % exit_code);
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

OMEGA_DEFINE_OID(OOCore,OID_InterProcessService,"{7E9E22E8-C0B0-43F9-9575-BFB1665CAE4A}");
OMEGA_DEFINE_OID(OOCore,OID_Surrogate,"{D063D32C-FB9A-004A-D2E5-BB5451808FF5}");
OMEGA_DEFINE_OID(OOCore,OID_SingleSurrogate,"{22DC1376-4905-D9DD-1B63-2096C487E5A3}");
