///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
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

// These functions are all raw, despite the fact they use non-POD pointers,
// as the pointers are no manipulated

#include <map>

namespace
{
	struct SafeHolder
	{
		OOBase::SpinLock m_lock;

		OOBase::HashTable<Omega::IObject*,const Omega::System::Internal::SafeShim*> m_obj_map;
		OOBase::HashTable<const Omega::System::Internal::SafeShim*,Omega::IObject*> m_shim_map;
	};
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_safe_holder__ctor,0,())
{
	SafeHolder* ret = new (std::nothrow) SafeHolder;
	if (!ret)
		OOBase_CallCriticalFailure(ERROR_OUTOFMEMORY);

	return ret;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_safe_holder__dctor,1,((in),void*,handle))
{
	delete static_cast<SafeHolder*>(handle);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(Omega::IObject*,OOCore_safe_holder_add1,3,((in),void*,handle,(in),const Omega::System::Internal::SafeShim*,shim,(in),Omega::IObject*,pObject))
{
	SafeHolder* pThis = static_cast<SafeHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	int err = pThis->m_shim_map.insert(shim,pObject);
	if (err == EEXIST)
		return *pThis->m_shim_map.find(shim);

	if (err == 0)
		err = pThis->m_obj_map.insert(pObject,shim);

	if (err != 0)
		OMEGA_THROW(err);

	return NULL;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const Omega::System::Internal::SafeShim*,OOCore_safe_holder_add2,3,((in),void*,handle,(in),Omega::IObject*,pObject,(in),const Omega::System::Internal::SafeShim*,shim))
{
	SafeHolder* pThis = static_cast<SafeHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	int err = pThis->m_obj_map.insert(pObject,shim);
	if (err == EEXIST)
		return *pThis->m_obj_map.find(pObject);
	
	if (err == 0)
		err = pThis->m_shim_map.insert(shim,pObject);
	
	if (err != 0)
		OMEGA_THROW(err);

	return NULL;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const Omega::System::Internal::SafeShim*,OOCore_safe_holder_find,2,((in),void*,handle,(in),Omega::IObject*,pObject))
{
	SafeHolder* pThis = static_cast<SafeHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	const Omega::System::Internal::SafeShim* ret = NULL;
	
	pThis->m_obj_map.find(pObject,ret);
	
	return ret;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_safe_holder_remove1,2,((in),void*,handle,(in),Omega::IObject*,pObject))
{
	SafeHolder* pThis = static_cast<SafeHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	const Omega::System::Internal::SafeShim* shim;
	if (pThis->m_obj_map.erase(pObject,&shim))
		pThis->m_shim_map.erase(shim);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_safe_holder_remove2,2,((in),void*,handle,(in),const Omega::System::Internal::SafeShim*,shim))
{
	SafeHolder* pThis = static_cast<SafeHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	Omega::IObject* pObject;
	if (pThis->m_shim_map.erase(shim,&pObject))
		pThis->m_obj_map.erase(pObject);
}
