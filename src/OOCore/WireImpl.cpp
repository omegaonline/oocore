///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOCore_precomp.h"

#include "./WireImpl.h"

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	struct wire_holder
	{
		struct pfns
		{
			System::MetaInfo::pfnCreateWireProxy pfnProxy;
			System::MetaInfo::pfnCreateWireStub pfnStub;
		};

		static wire_holder& instance()
		{
			static wire_holder i;
			return i;
		}

		std::map<guid_t,pfns> map;
	};
}

System::MetaInfo::IWireStub_Safe* OOCore::CreateWireStub(const guid_t& iid, System::MetaInfo::IWireStubController_Safe* pController, System::MetaInfo::IWireManager_Safe* pManager, System::MetaInfo::IObject_Safe* pObjS)
{
	wire_holder::pfns p;
	try
	{
		std::map<guid_t,wire_holder::pfns>::const_iterator i=wire_holder::instance().map.find(iid);
		if (i == wire_holder::instance().map.end())
			throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
		p = i->second;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	System::MetaInfo::IWireStub_Safe* pRet = 0;
	System::MetaInfo::IException_Safe* pSE = p.pfnStub(pController,pManager,pObjS,&pRet);

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);

	return pRet;
}

System::MetaInfo::IObject_Safe* OOCore::CreateWireProxy(const guid_t& iid, System::MetaInfo::IWireProxy_Safe* pProxy, System::MetaInfo::IWireManager_Safe* pManager)
{
	wire_holder::pfns p;
	try
	{
		std::map<guid_t,wire_holder::pfns>::const_iterator i=wire_holder::instance().map.find(iid);
		if (i == wire_holder::instance().map.end())
			throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
		p = i->second;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	System::MetaInfo::IObject_Safe* pRet = 0;
	System::MetaInfo::IException_Safe* pSE = p.pfnProxy(pProxy,pManager,&pRet);

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);

	return pRet;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_RegisterWireFactories,3,((in),const guid_t&,iid,(in),void*,pfnProxy,(in),void*,pfnStub))
{
	OOCore::wire_holder::pfns funcs;
	funcs.pfnProxy = (System::MetaInfo::pfnCreateWireProxy)(pfnProxy);
	funcs.pfnStub = (System::MetaInfo::pfnCreateWireStub)(pfnStub);

	try
	{
		OOCore::wire_holder::instance().map.insert(std::map<guid_t,OOCore::wire_holder::pfns>::value_type(iid,funcs));
	}
	catch (...)
	{}
}
