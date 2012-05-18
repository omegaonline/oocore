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

	void recurse_log_exception(Omega::IException* pE)
	{
		ObjectPtr<IException> ptrCause = pE->GetCause();
		if (ptrCause)
		{
			LOG_ERROR(("Cause: %s",ptrCause->GetDescription().c_str()));
			recurse_log_exception(ptrCause);
		}
	}

	int Run(const guid_t& oid)
	{
		int ret = EXIT_FAILURE;

		IException* pE = Omega::Initialize();
		if (pE)
		{
			ObjectPtr<IException> ptrE = pE;
			LOG_ERROR(("IException thrown: %s",ptrE->GetDescription().c_str()));
			recurse_log_exception(ptrE);
		}
		else
		{
			try
			{
				GetModule()->RegisterObjectFactory(oid);

				try
				{
					while (Omega::HandleRequest(30000) || GetModule()->HaveLocks())
					{}
				}
				catch (...)
				{
					GetModule()->UnregisterObjectFactories();
					throw;
				}

				GetModule()->UnregisterObjectFactories();

				ret = EXIT_SUCCESS;
			}
			catch (IException* pE)
			{
				ObjectPtr<IException> ptrE = pE;
				LOG_ERROR(("IException thrown: %s",ptrE->GetDescription().c_str()));
				recurse_log_exception(ptrE);
			}
			catch (...)
			{
				LOG_ERROR(("Unrecognised exception thrown"));
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
END_PROCESS_OBJECT_MAP()

OMEGA_DEFINE_OID(OOCore,OID_Surrogate,"{D063D32C-FB9A-004A-D2E5-BB5451808FF5}");
OMEGA_DEFINE_OID(OOCore,OID_SingleSurrogate,"{22DC1376-4905-D9DD-1B63-2096C487E5A3}");

void SingleSurrogateImpl::GetObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject)
{
	OOCore_GetObject(oid,clean_flags(flags),iid,pObject);
}

void SurrogateImpl::GetObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject)
{
	OOCore_GetObject(oid,clean_flags(flags),iid,pObject);
}

int Host::SingleSurrogate()
{
	return Run(OOCore::OID_SingleSurrogate);
}

int Host::MultipleSurrogate()
{
	return Run(OOCore::OID_Surrogate);
}
