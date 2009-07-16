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
			System::MetaInfo::pfnCreateWireProxy pfnProxy;
			System::MetaInfo::pfnCreateWireStub pfnStub;
		};

		OOBase::RWMutex            m_lock;
		std::multimap<guid_t,pfns> m_ps_map;
	};

	typedef OOBase::Singleton<wire_holder,OOCore::DLL> WIRE_HOLDER;
}

System::IStub* OOCore::CreateStub(const guid_t& iid, System::IStubController* pController, System::IMarshaller* pManager, IObject* pObj)
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

	System::IStub* pRet = 0;
	const System::MetaInfo::SafeShim* pSE = p.pfnStub(
		System::MetaInfo::marshal_info<System::IStubController*>::safe_type::coerce(pController),
		System::MetaInfo::marshal_info<System::IMarshaller*>::safe_type::coerce(pManager),
		System::MetaInfo::marshal_info<IObject*>::safe_type::coerce(pObj,iid),
		System::MetaInfo::marshal_info<System::IStub*&>::safe_type::coerce(pRet));

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);

	return pRet;
}

const System::MetaInfo::SafeShim* OOCore::CreateProxy(const guid_t& iid, System::IProxy* pProxy)
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

	const System::MetaInfo::SafeShim* pRet = 0;
	const System::MetaInfo::SafeShim* pSE = p.pfnProxy(
		System::MetaInfo::marshal_info<System::IProxy*>::safe_type::coerce(pProxy),&pRet);

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);

	return pRet;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_RegisterAutoProxyStubCreators,3,((in),const guid_t&,iid,(in),void*,pfnProxy,(in),void*,pfnStub))
{
	try
	{
		wire_holder::pfns funcs;
		funcs.pfnProxy = (System::MetaInfo::pfnCreateWireProxy)(pfnProxy);
		funcs.pfnStub = (System::MetaInfo::pfnCreateWireStub)(pfnStub);

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
			if (i->second.pfnProxy == (System::MetaInfo::pfnCreateWireProxy)(pfnProxy) &&
				i->second.pfnStub == (System::MetaInfo::pfnCreateWireStub)(pfnStub))
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
