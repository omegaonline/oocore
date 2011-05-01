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
// as the pointers are not manipulated

namespace
{
	struct QIRttiHolder
	{
		OOBase::SpinLock m_lock;

		OOBase::HashTable<Omega::guid_t,const Omega::System::Internal::qi_rtti*,OOBase::HeapAllocator,OOCore::GuidHash> m_map;
	};
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_qi_rtti_holder__ctor,0,())
{
	QIRttiHolder* ret = new (std::nothrow) QIRttiHolder;
	if (!ret)
		OOBase_CallCriticalFailure(ERROR_OUTOFMEMORY);

	return ret;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_qi_rtti_holder__dctor,1,((in),void*,handle))
{
	delete static_cast<QIRttiHolder*>(handle);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const Omega::System::Internal::qi_rtti*,OOCore_qi_rtti_holder_find,2,((in),void*,handle,(in),const Omega::guid_base_t*,iid))
{
	QIRttiHolder* pThis = static_cast<QIRttiHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	const Omega::System::Internal::qi_rtti* pRet = NULL;

	pThis->m_map.find(*iid,pRet);

	return pRet;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_qi_rtti_holder_insert,3,((in),void*,handle,(in),const Omega::guid_base_t*,iid,(in),const Omega::System::Internal::qi_rtti*,pRtti))
{
	QIRttiHolder* pThis = static_cast<QIRttiHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	int err = pThis->m_map.replace(*iid,pRtti);
	if (err != 0)
		OOBase_CallCriticalFailure(err);
}

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

namespace
{
	struct WireRttiHolder
	{
		OOBase::SpinLock m_lock;

		OOBase::HashTable<Omega::guid_t,const Omega::System::Internal::wire_rtti*,OOBase::HeapAllocator,OOCore::GuidHash> m_map;
	};
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_wire_rtti_holder__ctor,0,())
{
	WireRttiHolder* ret = new (std::nothrow) WireRttiHolder;
	if (!ret)
		OOBase_CallCriticalFailure(ERROR_OUTOFMEMORY);

	return ret;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_rtti_holder__dctor,1,((in),void*,handle))
{
	delete static_cast<WireRttiHolder*>(handle);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const Omega::System::Internal::wire_rtti*,OOCore_wire_rtti_holder_find,2,((in),void*,handle,(in),const Omega::guid_base_t*,iid))
{
	WireRttiHolder* pThis = static_cast<WireRttiHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	const Omega::System::Internal::wire_rtti* pRet = NULL;

	pThis->m_map.find(*iid,pRet);

	return pRet;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_rtti_holder_insert,3,((in),void*,handle,(in),const Omega::guid_base_t*,iid,(in),const Omega::System::Internal::wire_rtti*,pRtti))
{
	WireRttiHolder* pThis = static_cast<WireRttiHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	int err = pThis->m_map.replace(*iid,pRtti);
	if (err != 0)
		OOBase_CallCriticalFailure(err);
}

namespace
{
	struct WireHolder
	{
		OOBase::SpinLock m_lock;

		OOBase::HashTable<Omega::IObject*,Omega::IObject*> m_map;
	};
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_wire_holder__ctor,0,())
{
	WireHolder* ret = new (std::nothrow) WireHolder;
	if (!ret)
		OOBase_CallCriticalFailure(ERROR_OUTOFMEMORY);

	return ret;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_holder__dctor,1,((in),void*,handle))
{
	delete static_cast<WireHolder*>(handle);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(Omega::IObject*,OOCore_wire_holder_add,3,((in),void*,handle,(in),Omega::IObject*,pProxy,(in),Omega::IObject*,pObject))
{
	WireHolder* pThis = static_cast<WireHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	int err = pThis->m_map.insert(pProxy,pObject);
	if (err == EEXIST)
		return *pThis->m_map.find(pProxy);

	if (err != 0)
		OMEGA_THROW(err);
	
	return NULL;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(Omega::IObject*,OOCore_wire_holder_find,2,((in),void*,handle,(in),Omega::IObject*,pProxy))
{
	WireHolder* pThis = static_cast<WireHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	Omega::IObject* pObj = NULL;

	pThis->m_map.find(pProxy,pObj);
	
	return pObj;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_holder_remove,2,((in),void*,handle,(in),Omega::IObject*,pProxy))
{
	WireHolder* pThis = static_cast<WireHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	pThis->m_map.erase(pProxy);
}
