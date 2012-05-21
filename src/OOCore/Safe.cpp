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

		OOBase::HashTable<guid_t,const System::Internal::qi_rtti*,OOBase::HeapAllocator,OOCore::GuidHash> m_map;
	};
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_qi_rtti_holder__ctor,2,((in),void**,phandle,(in),Omega::Threading::SingletonCallback,pfn_init))
{
	void* pCur = OOBase::Atomic<void*>::CompareAndSwap(*phandle,NULL,(void*)1);
	if (!pCur)
	{
		*phandle = new (OOCore::throwing) QIRttiHolder;
		
		try
		{
			(*pfn_init)(phandle);
		}
		catch (...)
		{
			delete static_cast<QIRttiHolder*>(*phandle);
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

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_qi_rtti_holder__dctor,1,((in),void*,handle))
{
	delete static_cast<QIRttiHolder*>(handle);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const System::Internal::qi_rtti*,OOCore_qi_rtti_holder_find,2,((in),void*,handle,(in),const guid_base_t*,iid))
{
	QIRttiHolder* pThis = static_cast<QIRttiHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	const System::Internal::qi_rtti* pRet = NULL;

	pThis->m_map.find(*iid,pRet);

	return pRet;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_qi_rtti_holder_insert,3,((in),void*,handle,(in),const guid_base_t*,iid,(in),const System::Internal::qi_rtti*,pRtti))
{
	QIRttiHolder* pThis = static_cast<QIRttiHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	if (!pThis->m_map.exists(*iid))
	{
		int err = pThis->m_map.insert(*iid,pRtti);
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
	return new (OOBase::critical) SafeHolder;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_safe_holder__dctor,1,((in),void*,handle))
{
	delete static_cast<SafeHolder*>(handle);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(IObject*,OOCore_safe_holder_add1,3,((in),void*,handle,(in),const System::Internal::SafeShim*,shim,(in),IObject*,pObject))
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

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const System::Internal::SafeShim*,OOCore_safe_holder_add2,3,((in),void*,handle,(in),IObject*,pObject,(in),const System::Internal::SafeShim*,shim))
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

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const System::Internal::SafeShim*,OOCore_safe_holder_find,2,((in),void*,handle,(in),IObject*,pObject))
{
	SafeHolder* pThis = static_cast<SafeHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	const System::Internal::SafeShim* ret = NULL;
	
	pThis->m_obj_map.find(pObject,ret);
	
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
	struct WireRttiHolder
	{
		const System::Internal::wire_rtti* find(const guid_t& iid);
		void insert(const void* key, const guid_t& iid, const System::Internal::wire_rtti* pRtti);
		void remove(const void* key, const guid_t& iid);

	private:
		OOBase::RWMutex m_lock;

		struct wr_t
		{
			const void*                        key;
			const System::Internal::wire_rtti* wire_info;
		};

		OOBase::Table<guid_t,wr_t,OOBase::HeapAllocator> m_map;
	};
	typedef OOBase::Singleton<WireRttiHolder,OOCore::DLL> WIRE_RTTI_HELPER;
}

template class OOBase::Singleton<WireRttiHolder,OOCore::DLL>;

const System::Internal::wire_rtti* WireRttiHolder::find(const guid_t& iid)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	wr_t wr = {0};
	m_map.find(iid,wr);
	return wr.wire_info;
}

void WireRttiHolder::insert(const void* key, const guid_t& iid, const System::Internal::wire_rtti* pRtti)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	wr_t wr = { key, pRtti };

	int err = m_map.insert(iid,wr);
	if (err != 0)
		OMEGA_THROW(err);
}

void WireRttiHolder::remove(const void* key, const guid_t& iid)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	for (size_t i=m_map.find_first(iid); i < m_map.size() && *m_map.key_at(i)==iid;)
	{
		if (m_map.at(i)->key == key)
			m_map.remove_at(i);
		else
			++i;
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(const System::Internal::wire_rtti*,OOCore_wire_rtti_holder_find,1,((in),const guid_base_t*,iid))
{
	return WIRE_RTTI_HELPER::instance().find(*iid);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_rtti_holder_insert,3,((in),const void*,key,(in),const guid_base_t*,iid,(in),const System::Internal::wire_rtti*,pRtti))
{
	WIRE_RTTI_HELPER::instance().insert(key,*iid,pRtti);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_rtti_holder_remove,2,((in),const void*,key,(in),const guid_base_t*,iid))
{
	WIRE_RTTI_HELPER::instance().remove(key,*iid);
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
	return new (OOBase::critical) WireHolder;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_holder__dctor,1,((in),void*,handle))
{
	delete static_cast<WireHolder*>(handle);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(IObject*,OOCore_wire_holder_add,3,((in),void*,handle,(in),IObject*,pProxy,(in),IObject*,pObject))
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

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(IObject*,OOCore_wire_holder_find,2,((in),void*,handle,(in),IObject*,pProxy))
{
	WireHolder* pThis = static_cast<WireHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	IObject* pObj = NULL;

	pThis->m_map.find(pProxy,pObj);
	
	return pObj;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_wire_holder_remove,2,((in),void*,handle,(in),IObject*,pProxy))
{
	WireHolder* pThis = static_cast<WireHolder*>(handle);

	OOBase::Guard<OOBase::SpinLock> guard(pThis->m_lock);

	pThis->m_map.remove(pProxy);
}
