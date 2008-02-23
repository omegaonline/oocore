///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
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
		typedef System::MetaInfo::IException_Safe* (OMEGA_CALL *pfnCreateWireStub)(
			System::MetaInfo::marshal_info<System::MetaInfo::IWireStub*&>::safe_type::type retval,
			System::MetaInfo::marshal_info<System::MetaInfo::IWireStubController*>::safe_type::type pController, 
			System::MetaInfo::marshal_info<System::MetaInfo::IWireManager*>::safe_type::type pManager,
			System::MetaInfo::marshal_info<IObject*>::safe_type::type pObject);

		typedef System::MetaInfo::IException_Safe* (OMEGA_CALL *pfnCreateWireProxy)(
			System::MetaInfo::marshal_info<IObject*&>::safe_type::type retval,
			System::MetaInfo::marshal_info<System::MetaInfo::IWireProxy*>::safe_type::type pProxy,
			System::MetaInfo::marshal_info<System::MetaInfo::IWireManager*>::safe_type::type pManager);

		struct pfns
		{
			wire_holder::pfnCreateWireProxy pfnProxy;
			wire_holder::pfnCreateWireStub pfnStub;
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
		OMEGA_THROW(string_t(e.what(),false));
	}

	System::MetaInfo::IWireStub_Safe* pRet = 0;
	System::MetaInfo::IException_Safe* pSE = p.pfnStub(&pRet,pController,pManager,pObjS);

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
		OMEGA_THROW(string_t(e.what(),false));
	}

	System::MetaInfo::IObject_Safe* pRet = 0;
	System::MetaInfo::IException_Safe* pSE = p.pfnProxy(&pRet,pProxy,pManager);

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);

	return pRet;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_RegisterWireFactories,3,((in),const guid_t&,iid,(in),void*,pfnProxy,(in),void*,pfnStub))
{
	OOCore::wire_holder::pfns funcs;
	funcs.pfnProxy = (OOCore::wire_holder::pfnCreateWireProxy)(pfnProxy);
	funcs.pfnStub = (OOCore::wire_holder::pfnCreateWireStub)(pfnStub);

	try
	{
		OOCore::wire_holder::instance().map.insert(std::map<guid_t,OOCore::wire_holder::pfns>::value_type(iid,funcs));
	}
	catch (...)
	{}
}
