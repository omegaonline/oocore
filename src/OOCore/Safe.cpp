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

using namespace Omega;

namespace
{
	struct QIRttiHolder
	{
		OOBase::SpinLock m_lock;

		OOBase::HashTable<guid_t,const System::Internal::qi_rtti*,OOBase::CrtAllocator,OOCore::GuidHash> m_qi_map;
		OOBase::HashTable<guid_t,const System::Internal::wire_rtti*,OOBase::CrtAllocator,OOCore::GuidHash> m_wi_map;
	};
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rtti_holder__ctor,2,((in),void**,phandle,(in),Omega::Threading::SingletonCallback,pfn_init))
{
	void* pCur = OOBase::Atomic<void*>::CompareAndSwap(*phandle,NULL,(void*)1);
	if (!pCur)
	{
		QIRttiHolder* qi = NULL;
		if (!OOBase::CrtAllocator::allocate_new(qi))
			OOBase_CallCriticalFailure(ERROR_OUTOFMEMORY);

		*phandle = qi;
		
		try
		{
			(*pfn_init)(phandle);
		}
		catch (...)
		{
			OOBase::CrtAllocator::delete_free(static_cast<QIRttiHolder*>(*phandle));
			*phandle = NULL;
			throw;
		}
	}

	while (pCur == (void*)1)
	{
		OOBase::Thread::yield();
		pCur = *phandle;
	}	
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rtti_holder__dctor,1,((in),void*,handle))
{
	OOBase::CrtAllocator::delete_free(static_cast<QIRttiHolder*>(handle));
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const System::Internal::qi_rtti*,OOCore_rtti_holder_find_qi,2,((in),void*,handle,(in),const guid_base_t*,iid))
{
	QIRttiHolder* pThis = static_cast<QIRttiHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	const System::Internal::qi_rtti* pRet = NULL;
	OOBase::HashTable<guid_t,const System::Internal::qi_rtti*,OOBase::CrtAllocator,OOCore::GuidHash>::iterator i = pThis->m_qi_map.find(guid_t(*iid));
	if (i != pThis->m_qi_map.end())
		pRet = i->value;

	return pRet;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const System::Internal::wire_rtti*,OOCore_rtti_holder_find_wi,2,((in),void*,handle,(in),const guid_base_t*,iid))
{
	QIRttiHolder* pThis = static_cast<QIRttiHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	const System::Internal::wire_rtti* pRet = NULL;
	OOBase::HashTable<guid_t,const System::Internal::wire_rtti*,OOBase::CrtAllocator,OOCore::GuidHash>::iterator i = pThis->m_wi_map.find(guid_t(*iid));
	if (i != pThis->m_wi_map.end())
		pRet = i->value;

	return pRet;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rtti_holder_insert_qi,3,((in),void*,handle,(in),const guid_base_t*,iid,(in),const System::Internal::qi_rtti*,pRtti))
{
	QIRttiHolder* pThis = static_cast<QIRttiHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	if (!pThis->m_qi_map.exists(guid_t(*iid)))
	{
		int err = pThis->m_qi_map.insert(guid_t(*iid),pRtti);
		if (err != 0)
			OOBase_CallCriticalFailure(err);
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rtti_holder_insert_wi,3,((in),void*,handle,(in),const guid_base_t*,iid,(in),const System::Internal::wire_rtti*,pRtti))
{
	QIRttiHolder* pThis = static_cast<QIRttiHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	if (!pThis->m_wi_map.exists(guid_t(*iid)))
	{
		int err = pThis->m_wi_map.insert(guid_t(*iid),pRtti);
		if (err != 0)
			OOBase_CallCriticalFailure(err);
	}
}

namespace
{
	struct SafeHolder
	{
		OOBase::SpinLock m_lock;

		OOBase::HashTable<IObject*,const System::Internal::SafeShim*> m_obj_map;
		OOBase::HashTable<const System::Internal::SafeShim*,IObject*> m_shim_map;
	};
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_safe_holder__ctor,0,())
{
	SafeHolder* s = NULL;
	if (!OOBase::CrtAllocator::allocate_new(s))
		OOBase_CallCriticalFailure(ERROR_OUTOFMEMORY);
	return s;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_safe_holder__dctor,1,((in),void*,handle))
{
	OOBase::CrtAllocator::delete_free(static_cast<SafeHolder*>(handle));
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(IObject*,OOCore_safe_holder_add1,3,((in),void*,handle,(in),const System::Internal::SafeShim*,shim,(in),IObject*,pObject))
{
	SafeHolder* pThis = static_cast<SafeHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	int err = 0;
	OOBase::HashTable<const System::Internal::SafeShim*,IObject*>::iterator i = pThis->m_shim_map.insert(shim,pObject,err);
	if (err == EEXIST)
		return i->value;

	if (err == 0)
		err = pThis->m_obj_map.insert(pObject,shim);

	if (err != 0)
		OMEGA_THROW(err);

	return NULL;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const System::Internal::SafeShim*,OOCore_safe_holder_add2,3,((in),void*,handle,(in),IObject*,pObject,(in),const System::Internal::SafeShim*,shim))
{
	SafeHolder* pThis = static_cast<SafeHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	int err = 0;
	OOBase::HashTable<IObject*,const System::Internal::SafeShim*>::iterator i = pThis->m_obj_map.insert(pObject,shim,err);
	if (err == EEXIST)
		return i->value;
	
	if (err == 0)
		err = pThis->m_shim_map.insert(shim,pObject);
	
	if (err != 0)
		OMEGA_THROW(err);

	return NULL;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const System::Internal::SafeShim*,OOCore_safe_holder_find,2,((in),void*,handle,(in),IObject*,pObject))
{
	SafeHolder* pThis = static_cast<SafeHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	const System::Internal::SafeShim* ret = NULL;
	OOBase::HashTable<IObject*,const System::Internal::SafeShim*>::iterator i = pThis->m_obj_map.find(pObject);
	if (i != pThis->m_obj_map.end())
		ret = i->value;
	
	return ret;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_safe_holder_remove1,2,((in),void*,handle,(in),IObject*,pObject))
{
	SafeHolder* pThis = static_cast<SafeHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	const System::Internal::SafeShim* shim;
	if (pThis->m_obj_map.remove(pObject,&shim))
		pThis->m_shim_map.remove(shim);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_safe_holder_remove2,2,((in),void*,handle,(in),const System::Internal::SafeShim*,shim))
{
	SafeHolder* pThis = static_cast<SafeHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	IObject* pObject;
	if (pThis->m_shim_map.remove(shim,&pObject))
		pThis->m_obj_map.remove(pObject);
}

namespace
{
	struct WireHolder
	{
		OOBase::SpinLock m_lock;

		OOBase::HashTable<IObject*,IObject*> m_map;
	};
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_wire_holder__ctor,0,())
{
	WireHolder* w = NULL;
	if (!OOBase::CrtAllocator::allocate_new(w))
		OOBase_CallCriticalFailure(ERROR_OUTOFMEMORY);
	return w;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_holder__dctor,1,((in),void*,handle))
{
	OOBase::CrtAllocator::delete_free(static_cast<WireHolder*>(handle));
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(IObject*,OOCore_wire_holder_add,3,((in),void*,handle,(in),IObject*,pProxy,(in),IObject*,pObject))
{
	WireHolder* pThis = static_cast<WireHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	int err = 0;
	OOBase::HashTable<IObject*,IObject*>::iterator i = pThis->m_map.insert(pProxy,pObject,err);
	if (err == EEXIST)
		return i->value;

	if (err != 0)
		OMEGA_THROW(err);
	
	return NULL;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(IObject*,OOCore_wire_holder_find,2,((in),void*,handle,(in),IObject*,pProxy))
{
	WireHolder* pThis = static_cast<WireHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	IObject* pObj = NULL;
	OOBase::HashTable<IObject*,IObject*>::iterator i = pThis->m_map.find(pProxy);
	if (i != pThis->m_map.end())
		pObj = i->value;
	
	return pObj;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_holder_remove,2,((in),void*,handle,(in),IObject*,pProxy))
{
	WireHolder* pThis = static_cast<WireHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	pThis->m_map.remove(pProxy);
}
