///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2012 Rick Taylor
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

#include "OOServer_Host.h"

#include <stdlib.h>

using namespace Omega;
using namespace OTL;

namespace
{
	class SingleSurrogateImpl :
			public ObjectBase,
			public AutoObjectFactory<SingleSurrogateImpl,&OOCore::OID_SingleSurrogate,Activation::UserScope | Activation::SingleUse>,
			public Remoting::ISurrogate
	{
	public:
		SingleSurrogateImpl()
		{ }

		BEGIN_INTERFACE_MAP(SingleSurrogateImpl)
			INTERFACE_ENTRY(Remoting::ISurrogate)
		END_INTERFACE_MAP()

	// ISurrogate members
	public:
		void GetObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject);
	};

	class SurrogateImpl :
			public ObjectBase,
			public AutoObjectFactorySingleton<SurrogateImpl,&OOCore::OID_Surrogate>,
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
		System::IService* Start(const string_t& strPipe, const string_t& strName, Registry::IKey* pKey, const string_t& strSecret);
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

	int worker(void* p)
	{
		try
		{
			while (Omega::HandleRequest(*static_cast<uint32_t*>(p)) || GetModule()->HaveLocks() || !Omega::CanUnload())
			{}

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

	int Run(const guid_t& oid, uint32_t msecs)
	{
		int ret = EXIT_FAILURE;

		IException* pE = Omega::Initialize();
		if (pE)
		{
			ObjectPtr<IException> ptrE = pE;
			LOG_ERROR(("IException thrown: %s",recurse_log_exception(ptrE).c_str()));
		}
		else
		{
			try
			{
				GetModule()->RegisterObjectFactory(oid);

				OOBase::ThreadPool pool;
				int err = pool.run(&worker,&msecs,2);
				if (err)
					LOG_ERROR(("Failed to start thread pool: %s",OOBase::system_error_text(err)));
				else
					worker(&msecs);

				pool.join();

				GetModule()->UnregisterObjectFactories();

				ret = EXIT_SUCCESS;
			}
			catch (IException* pE)
			{
				ObjectPtr<IException> ptrE = pE;
				LOG_ERROR(("IException thrown: %s",recurse_log_exception(ptrE).c_str()));
			}

			Omega::Uninitialize();
		}

		return ret;
	}

	Activation::Flags_t clean_flags(Activation::Flags_t flags)
	{
		Activation::Flags_t new_flags = Activation::Library;
		if (flags & Activation::RemoteActivation)
			new_flags |= Activation::RemoteActivation;

		return new_flags;
	}
}

BEGIN_PROCESS_OBJECT_MAP()
	OBJECT_MAP_ENTRY(SingleSurrogateImpl)
	OBJECT_MAP_ENTRY(SurrogateImpl)
	OBJECT_MAP_ENTRY(ServiceManagerImpl)
END_PROCESS_OBJECT_MAP()

const Omega::guid_t OOCore::OID_Surrogate("{D063D32C-FB9A-004A-D2E5-BB5451808FF5}");
const Omega::guid_t OOCore::OID_SingleSurrogate("{22DC1376-4905-D9DD-1B63-2096C487E5A3}");
const Omega::guid_t OOCore::OID_ServiceManager("{1ACC3273-8FB3-9741-E7E6-1CD4C6150FB2}");

void SingleSurrogateImpl::GetObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject)
{
	OOCore_GetObject(oid,clean_flags(flags),iid,pObject);
}

void SurrogateImpl::GetObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject)
{
	OOCore_GetObject(oid,clean_flags(flags),iid,pObject);
}

System::IService* ServiceManagerImpl::Start(const string_t& strPipe, const string_t& strName, Registry::IKey* pKey, const string_t& strSecret)
{
	return Host::StartService(strPipe,strName,pKey,strSecret);
}

int Host::SingleSurrogate()
{
	return Run(OOCore::OID_SingleSurrogate,30000);
}

int Host::MultipleSurrogate()
{
	return Run(OOCore::OID_Surrogate,30000);
}

int Host::ServiceStart()
{
	return Run(OOCore::OID_ServiceManager,0xFFFFFFFF);
}
