///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
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

using namespace Omega;
using namespace OTL;

namespace 
{
	struct proxy_holder
	{
		OOBase::RWMutex             m_lock;
		std::map<const void*,void*> m_map;
	};
	typedef OOBase::Singleton<proxy_holder> PROXY_HOLDER;

	struct stub_holder
	{
		OOBase::RWMutex       m_lock;
		std::map<void*,void*> m_map;
	};
	typedef OOBase::Singleton<stub_holder> STUB_HOLDER;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_safe_proxy_find,1,((in),const void*,shim))
{
	try
	{
		proxy_holder* pI = PROXY_HOLDER::instance();

		OOBase::ReadGuard<OOBase::RWMutex> guard(pI->m_lock);

		std::map<const void*,void*>::const_iterator i=pI->m_map.find(shim);
		if (i != pI->m_map.end())
			return i->second;
		
		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_safe_proxy_add,2,((in),const void*,shim,(in),void*,pProxy))
{
	try
	{
		proxy_holder* pI = PROXY_HOLDER::instance();

		OOBase::Guard<OOBase::RWMutex> guard(pI->m_lock);

		std::pair<std::map<const void*,void*>::iterator,bool> p = pI->m_map.insert(std::map<const void*,void*>::value_type(shim,pProxy));
		if (!p.second)
			return p.first->second;
		
		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_safe_proxy_remove,1,((in),const void*,shim))
{
	try
	{
		proxy_holder* pI = PROXY_HOLDER::instance();

		OOBase::Guard<OOBase::RWMutex> guard(pI->m_lock);

		pI->m_map.erase(shim);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_safe_stub_find,1,((in),void*,pObject))
{
	try
	{
		stub_holder* pI = STUB_HOLDER::instance();

		OOBase::ReadGuard<OOBase::RWMutex> guard(pI->m_lock);

		std::map<void*,void*>::const_iterator i=pI->m_map.find(pObject);
		if (i != pI->m_map.end())
			return i->second;
		
		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_safe_stub_add,2,((in),void*,pObject,(in),void*,pStub))
{
	try
	{
		stub_holder* pI = STUB_HOLDER::instance();

		OOBase::Guard<OOBase::RWMutex> guard(pI->m_lock);

		std::pair<std::map<void*,void*>::iterator,bool> p = pI->m_map.insert(std::map<void*,void*>::value_type(pObject,pStub));
		if (!p.second)
			return p.first->second;
		
		return 0;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_safe_stub_remove,1,((in),void*,pObject))
{
	try
	{
		stub_holder* pI = STUB_HOLDER::instance();

		OOBase::Guard<OOBase::RWMutex> guard(pI->m_lock);

		pI->m_map.erase(pObject);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}
