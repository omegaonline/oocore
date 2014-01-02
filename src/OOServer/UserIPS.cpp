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
#include "UserRegistry.h"

using namespace Omega;
using namespace OTL;

namespace
{
	class ROTNotifier : Activation::IRunningObjectTableNotify
	{
	public:
		ROTNotifier(ObjectPtr<Notify::INotifier> ptrNotify, const guid_t& oid) :
				m_ptrNotify(ptrNotify), m_cookie(0), m_oid(oid)
		{
			m_cookie = m_ptrNotify->RegisterNotify(OMEGA_GUIDOF(Activation::IRunningObjectTableNotify),this);
		}

		virtual ~ROTNotifier()
		{
			if (m_cookie)
				m_ptrNotify->UnregisterNotify(m_cookie);
		}

		bool wait(const OOBase::Timeout& timeout)
		{
			bool res = false;
			if (!m_future.wait(res,false,timeout))
				return false;

			return true;
		}

	private:
		ObjectPtr<Notify::INotifier> m_ptrNotify;
		uint32_t                     m_cookie;
		OOBase::Future<bool>         m_future;
		const guid_t&                m_oid;

	// IObject members
	public:
		void AddRef()
		{}

		void Release()
		{}

		IObject* QueryInterface(const guid_t& iid)
		{
			if (iid == OMEGA_GUIDOF(IObject) || iid == OMEGA_GUIDOF(Activation::IRunningObjectTableNotify))
				return this;

			return NULL;
		}

	// IRunningObjectTableNotify members
	public:
		void OnRegisterObject(const any_t& oid, Activation::RegisterFlags_t)
		{
			if (oid == m_oid)
				m_future.signal(true);
		}

		void OnRevokeObject(const any_t&, Activation::RegisterFlags_t)
		{}
	};
}

void User::InterProcessService::init(Remoting::IObjectManager* pOMSB)
{
	if (pOMSB)
	{
		void* TODO; // TODO, Get this another way!

		// Create a proxy to the server interface
		IObject* pIPS = NULL;
		pOMSB->GetRemoteInstance(OOCore::OID_InterProcessService,Activation::Library | Activation::DontLaunch,OMEGA_GUIDOF(OOCore::IInterProcessService),pIPS);
		m_ptrSBIPS = static_cast<OOCore::IInterProcessService*>(pIPS);
	}

	// Create the ROT
	m_ptrROT = ObjectImpl<User::RunningObjectTable>::CreateObject();
	try
	{
		m_ptrROT->init(m_ptrSBIPS);
	}
	catch (...)
	{
		m_ptrROT.Release();
		throw;
	}
}

Activation::IRunningObjectTable* User::InterProcessService::GetRunningObjectTable()
{
	return m_ptrROT.AddRef();
}

string_t User::InterProcessService::GetSurrogateProcess(const guid_t& oid)
{
	string_t strProcess;
	
#if !defined(_WIN32)
	Manager::instance()->get_root_config_arg("binary_path",strProcess);
	strProcess += "oosvrhost";
#endif

	if (User::is_debug())
		strProcess += " --debug";

	if (oid == OOCore::OID_Surrogate)
		strProcess += " --surrogate";
	else if (oid == OOCore::OID_ServiceManager)
		strProcess += " --service";
	else
		strProcess.Clear();

	return strProcess;
}

void User::InterProcessService::LaunchObjectApp(const guid_t& oid, const guid_t& iid, Activation::Flags_t flags, IObject*& pObject)
{
	// Forward to sandbox if required
	if (m_ptrSBIPS && (flags & 0xF) >= Activation::Sandbox)
	{
		m_ptrSBIPS->LaunchObjectApp(oid,iid,flags,pObject);
		return;
	}

	// Find the OID key...
	ObjectPtr<Omega::Registry::IKey> ptrLU = ObjectPtr<Omega::Registry::IOverlayKeyFactory>(Omega::Registry::OID_OverlayKeyFactory)->Overlay("Local User","All Users");
	ObjectPtr<Omega::Registry::IKey> ptrKey = ptrLU->OpenKey("Objects/OIDs");

	bool is_host_process = false;
	string_t strProcess,strWorkingDir;

	OOBase::Environment::env_table_t tabEnv;

	// Find the name of the executable to run...
	ptrKey = ptrKey->OpenKey(oid.ToString());
	string_t strAppName = ptrKey->GetValue(string_t::constant("Application")).cast<string_t>();
	ptrKey = ptrLU->OpenKey("Applications/" + strAppName + "/Activation");

	if (ptrKey->IsValue(string_t::constant("SystemHost")))
	{
		strProcess = GetSurrogateProcess(oid);
		is_host_process = !strProcess.IsEmpty();
		if (is_host_process)
			OOBase::Logger::log(OOBase::Logger::Information,"Using host process for OID %s",oid.ToString().c_str());
	}

	if (!is_host_process)
	{
		strProcess = ptrKey->GetValue(string_t::constant("Path")).cast<string_t>();
		if (strProcess.IsEmpty() || User::Process::is_invalid_path(strProcess))
			throw IAccessDeniedException::Create(string_t::constant("Invalid path \"{0}\" in application activation registry value.") % strProcess);
	}

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

	// Get the environment settings
	OOBase::Environment::env_table_t tabSysEnv;
	int err = OOBase::Environment::get_current(tabSysEnv);
	if (err)
		OMEGA_THROW(err);

	err = OOBase::Environment::substitute(tabEnv,tabSysEnv);
	if (err)
		OMEGA_THROW(err);

	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	int exit_code = 0;
	OOBase::SharedPtr<User::Process> ptrProcess;
	if (m_mapInProgress.find(strProcess,ptrProcess) && !ptrProcess->is_running(exit_code))
	{
		m_mapInProgress.remove(strProcess);
		ptrProcess.reset();
	}

	if (!ptrProcess)
	{
		// Create a new process
		ptrProcess = Manager::instance()->exec(strProcess,strWorkingDir,is_host_process,tabEnv);

		err = m_mapInProgress.insert(strProcess,ptrProcess);
		if (err != 0)
			OMEGA_THROW(err);
	}

	guard.release();

	ObjectPtr<Notify::INotifier> ptrNotify = m_ptrROT.QueryInterface<Notify::INotifier>();
	if (!ptrNotify)
		throw OOCore_INotFoundException_MissingIID(OMEGA_GUIDOF(Notify::INotifier));

	// Use a notifier with a OOBase::Future
	ROTNotifier notifier(ptrNotify,oid);

	// Wait for the process to start and register its parts...
	OOBase::Timeout wait(30,0);

	// Wait for the oid to be registered
	if (notifier.wait(wait))
	{
		// Get the object from the ROT
		m_ptrROT->GetObject(oid,iid,pObject);
	}

	// Check the process is still alive
	bool bRunning = ptrProcess->is_running(exit_code);

	// Remove from the map
	guard.acquire();
	m_mapInProgress.remove(strProcess);
	guard.release();

	if (!pObject)
	{
		if (bRunning)
		{
			OOBase::Logger::log(OOBase::Logger::Debug,"Given up waiting for process %s",strProcess.c_str());

			ptrProcess->kill();

			throw ITimeoutException::Create();
		}

		OOBase::Logger::log(OOBase::Logger::Debug,"Process %s exited with code %d",strProcess.c_str(),exit_code);

		throw INotFoundException::Create(string_t::constant("The process '{0}' terminated unexpectedly with exit code {1}") % strProcess % exit_code);
	}
}

const Omega::guid_t OOCore::OID_InterProcessService("{7E9E22E8-C0B0-43F9-9575-BFB1665CAE4A}");
const Omega::guid_t OOCore::OID_Surrogate("{D063D32C-FB9A-004A-D2E5-BB5451808FF5}");
const Omega::guid_t OOCore::OID_ServiceManager("{1ACC3273-8FB3-9741-E7E6-1CD4C6150FB2}");
