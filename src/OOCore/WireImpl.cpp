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
			System::MetaInfo::pfnCreateProxy pfnProxy;
			System::MetaInfo::pfnCreateStub pfnStub;
		};

		static wire_holder& instance()
		{
			static wire_holder i;
			return i;
		}

		std::map<guid_t,pfns> ps_map;
		std::map<guid_t,System::MetaInfo::pfnCreateTypeInfo> ti_map;
	};
}

System::MetaInfo::IStub_Safe* OOCore::CreateStub(const guid_t& iid, System::MetaInfo::IStubController_Safe* pController, System::MetaInfo::IMarshaller_Safe* pManager, System::MetaInfo::IObject_Safe* pObjS)
{
	wire_holder::pfns p;
	try
	{
		wire_holder& instance = wire_holder::instance();
		std::map<guid_t,wire_holder::pfns>::const_iterator i=instance.ps_map.find(iid);
		if (i == instance.ps_map.end())
			throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
		p = i->second;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	System::MetaInfo::IStub_Safe* pRet = 0;
	System::MetaInfo::IException_Safe* pSE = p.pfnStub(pController,pManager,pObjS,&pRet);

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);

	return pRet;
}

System::MetaInfo::IObject_Safe* OOCore::CreateProxy(const guid_t& iid, System::MetaInfo::IProxy_Safe* pProxy, System::MetaInfo::IMarshaller_Safe* pManager)
{
	wire_holder::pfns p;
	try
	{
		wire_holder& instance = wire_holder::instance();
		std::map<guid_t,wire_holder::pfns>::const_iterator i=instance.ps_map.find(iid);
		if (i == instance.ps_map.end())
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

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_RegisterAutoProxyStubCreators,3,((in),const guid_t&,iid,(in),void*,pfnProxy,(in),void*,pfnStub))
{
	OOCore::wire_holder::pfns funcs;
	funcs.pfnProxy = (System::MetaInfo::pfnCreateProxy)(pfnProxy);
	funcs.pfnStub = (System::MetaInfo::pfnCreateStub)(pfnStub);

	try
	{
		OOCore::wire_holder::instance().ps_map.insert(std::map<guid_t,OOCore::wire_holder::pfns>::value_type(iid,funcs));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

System::MetaInfo::ITypeInfo_Safe* OOCore::GetTypeInfo(const guid_t& iid)
{
	System::MetaInfo::pfnCreateTypeInfo t;
	try
	{
		wire_holder& instance = wire_holder::instance();
		std::map<guid_t,System::MetaInfo::pfnCreateTypeInfo>::const_iterator i=instance.ti_map.find(iid);
		if (i == instance.ti_map.end())
			throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
		t = i->second;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	System::MetaInfo::ITypeInfo_Safe* pRet = 0;
	System::MetaInfo::IException_Safe* pSE = t(&pRet);

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);

	return pRet;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_RegisterAutoTypeInfo,2,((in),const Omega::guid_t&,iid,(in),void*,pfnTypeInfo))
{
	try
	{
		OOCore::wire_holder::instance().ti_map.insert(std::map<guid_t,System::MetaInfo::pfnCreateTypeInfo>::value_type(iid,(System::MetaInfo::pfnCreateTypeInfo)pfnTypeInfo));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}