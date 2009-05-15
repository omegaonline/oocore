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

#include "WireImpl.h"

using namespace Omega;
using namespace OTL;

namespace 
{
	struct wire_holder
	{
		struct pfns
		{
			System::MetaInfo::pfnCreateProxy pfnProxy;
			System::MetaInfo::pfnCreateStub pfnStub;
		};

		OOBase::RWMutex            m_lock;
		std::multimap<guid_t,pfns> m_ps_map;
		//std::map<guid_t,System::MetaInfo::pfnCreateTypeInfo> m_ti_map;
	};

	typedef OOBase::Singleton<wire_holder> WIRE_HOLDER;
}

System::MetaInfo::IStub_Safe* OOCore::CreateStub(const guid_t& iid, System::MetaInfo::IStubController_Safe* pController, System::MetaInfo::IMarshaller_Safe* pManager, System::MetaInfo::IObject_Safe* pObjS)
{
	wire_holder::pfns p = {0,0};
	try
	{
		wire_holder* instance = WIRE_HOLDER::instance();

		OOBase::ReadGuard<OOBase::RWMutex> guard(instance->m_lock);

		std::multimap<guid_t,wire_holder::pfns>::const_iterator i=instance->m_ps_map.find(iid);
		if (i == instance->m_ps_map.end())
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
	wire_holder::pfns p = {0,0};
	try
	{
		wire_holder* instance = WIRE_HOLDER::instance();

		OOBase::ReadGuard<OOBase::RWMutex> guard(instance->m_lock);

		std::multimap<guid_t,wire_holder::pfns>::const_iterator i=instance->m_ps_map.find(iid);
		if (i == instance->m_ps_map.end())
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

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_RegisterAutoProxyStubCreators,3,((in),const guid_t&,iid,(in),void*,pfnProxy,(in),void*,pfnStub))
{
	try
	{
		wire_holder::pfns funcs;
		funcs.pfnProxy = (System::MetaInfo::pfnCreateProxy)(pfnProxy);
		funcs.pfnStub = (System::MetaInfo::pfnCreateStub)(pfnStub);

		wire_holder* instance = WIRE_HOLDER::instance();

		OOBase::Guard<OOBase::RWMutex> guard(instance->m_lock);

		instance->m_ps_map.insert(std::multimap<guid_t,wire_holder::pfns>::value_type(iid,funcs));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_UnregisterAutoProxyStubCreators,3,((in),const guid_t&,iid,(in),void*,pfnProxy,(in),void*,pfnStub))
{
	try
	{
		wire_holder* instance = WIRE_HOLDER::instance();

		OOBase::Guard<OOBase::RWMutex> guard(instance->m_lock);

		for (std::multimap<guid_t,wire_holder::pfns>::iterator i=instance->m_ps_map.find(iid);i!=instance->m_ps_map.end() && i->first==iid;)
		{
			if (i->second.pfnProxy == (System::MetaInfo::pfnCreateProxy)(pfnProxy) &&
				i->second.pfnStub == (System::MetaInfo::pfnCreateStub)(pfnStub))
			{
				instance->m_ps_map.erase(i++);
			}
			else
			{
				++i;
			}
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}
