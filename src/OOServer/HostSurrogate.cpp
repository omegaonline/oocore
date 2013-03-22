///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2012 Rick Taylor
//
// This file is part of OOSvrHost, the Omega Online user host application.
//
// OOSvrHost is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrHost is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOSvrHost.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer_Host.h"

#include <stdlib.h>

using namespace Omega;
using namespace OTL;

namespace
{
	class SurrogateImpl :
			public ObjectBase,
			public AutoObjectFactory<SurrogateImpl,&OOCore::OID_Surrogate,Activation::UserScope | Activation::SingleUse>,
			public Remoting::ISurrogate
	{
	public:
		SurrogateImpl()
		{ }

		BEGIN_INTERFACE_MAP(SurrogateImpl)
			INTERFACE_ENTRY(Remoting::ISurrogate)
		END_INTERFACE_MAP()

	// ISurrogate members
	public:
		void GetObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject);
	};

	class ServiceManagerImpl :
			public ObjectBase,
			public AutoObjectFactory<ServiceManagerImpl,&OOCore::OID_ServiceManager,Activation::UserScope | Activation::SingleUse>,
			public OOCore::IServiceManager
	{
	public:
		ServiceManagerImpl()
		{ }

		BEGIN_INTERFACE_MAP(ServiceManagerImpl)
			INTERFACE_ENTRY(OOCore::IServiceManager)
		END_INTERFACE_MAP()

	// IServiceManager members
	public:
		System::IService* Create(const any_t& oid);
		void Start(System::IService* pService, const string_t& strName, const string_t& strPipe, Registry::IKey* pKey, const string_t& strSecret);
	};

	string_t recurse_log_exception(Omega::IException* pE)
	{
		string_t msg = pE->GetDescription();
		if (!msg.IsEmpty() && msg[msg.Length()-1] != '.')
			msg += ".";

		ObjectPtr<IException> ptrCause = pE->GetCause();
		if (ptrCause)
			msg += "\nCause: " + recurse_log_exception(ptrCause);

		return msg;
	}

	int worker(void*)
	{
		// Initially wait 15 seconds for 1st message
		uint32_t msecs = 15000;
		try
		{
			while (Omega::HandleRequest(msecs) || GetModuleBase()->HaveLocks() || !Omega::CanUnload())
			{
				// Once we have the first message, we can then wait a very short time
				msecs = 500;
			}

			return 0;
		}
		catch (IException* pE)
		{
			ObjectPtr<IException> ptrE = pE;
			LOG_ERROR_RETURN(("IException thrown: %s",recurse_log_exception(ptrE).c_str()),-1);
		}
		catch (...)
		{
			LOG_ERROR_RETURN(("Unrecognised exception thrown"),-1);
		}
	}

	int Run(const guid_t& oid)
	{
		int ret = EXIT_FAILURE;

		try
		{
			Omega::Initialize();

			GetModuleBase()->RegisterObjectFactory(oid);

			OOBase::ThreadPool pool;
			int err = pool.run(&worker,NULL,2);
			if (err)
				LOG_ERROR(("Failed to start thread pool: %s",OOBase::system_error_text(err)));
			else
				worker(NULL);

			pool.join();

			GetModuleBase()->UnregisterObjectFactories();

			Omega::Uninitialize();

			ret = EXIT_SUCCESS;
		}
		catch (IException* pE)
		{
			ObjectPtr<IException> ptrE = pE;
			LOG_ERROR(("IException thrown: %s",recurse_log_exception(ptrE).c_str()));
		}

		return ret;
	}
}

BEGIN_PROCESS_OBJECT_MAP()
	OBJECT_MAP_ENTRY(SurrogateImpl)
	OBJECT_MAP_ENTRY(ServiceManagerImpl)
END_PROCESS_OBJECT_MAP()

const Omega::guid_t OOCore::OID_Surrogate("{D063D32C-FB9A-004A-D2E5-BB5451808FF5}");
const Omega::guid_t OOCore::OID_ServiceManager("{1ACC3273-8FB3-9741-E7E6-1CD4C6150FB2}");

void SurrogateImpl::GetObject(const guid_t& oid, Activation::Flags_t, const guid_t& iid, IObject*& pObject)
{
	OOCore_GetObject(oid,Activation::Library,iid,pObject);
}

System::IService* ServiceManagerImpl::Create(const any_t& oid)
{
	return ObjectPtr<System::IService>(oid).AddRef();
}

void ServiceManagerImpl::Start(System::IService* pService, const string_t& strName, const string_t& strPipe, Registry::IKey* pKey, const string_t& strSecret)
{
	try
	{
		Host::StartService(pService,strName,strPipe,pKey,strSecret);
	}
	catch (IException* pE)
	{
		ObjectPtr<IException> ptrE = pE;
		LOG_ERROR(("An exception occurred in service '%s': %s",strName.c_str(),recurse_log_exception(ptrE).c_str()));
	}
}

int Host::Surrogate()
{
	return Run(OOCore::OID_Surrogate);
}

int Host::ServiceStart()
{
	return Run(OOCore::OID_ServiceManager);
}
