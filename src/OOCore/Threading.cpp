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

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_sngtn_once,2,((in),void**,val,(in),Omega::Threading::SingletonCallback,pfn_init))
{
	// The value pointed to is definitely volatile under race conditions
	volatile void* pVal = *val;

	// Do a double lock... this is so we can call it more than once
	if (!pVal)
	{
		// This singleton is race start safe...
		OOBase::Guard<OOBase::SpinLock> guard(OOBase::Singleton<OOBase::SpinLock,OOCore::DLL>::instance());

		if (!pVal)
		{
			// Call the init function
			const Omega::System::Internal::SafeShim* pE = (*pfn_init)(val);
			if (pE)
				Omega::System::Internal::throw_correct_exception(pE);
		}
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_cs__ctor,0,())
{
	void* r = new (std::nothrow) OOBase::Mutex();
	if (!r)
		OMEGA_THROW_NOMEM();

	return r;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_cs__dctor,1,((in),void*,m1))
{
	delete static_cast<OOBase::Mutex*>(m1);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_cs_lock,1,((in),void*,m1))
{
	static_cast<OOBase::Mutex*>(m1)->acquire();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_cs_unlock,1,((in),void*,m1))
{
	static_cast<OOBase::Mutex*>(m1)->release();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_rw_lock__ctor,0,())
{
	void* r = new (std::nothrow) OOBase::RWMutex();
	if (!r)
		OMEGA_THROW_NOMEM();

	return r;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock__dctor,1,((in),void*,m1))
{
	delete static_cast<OOBase::RWMutex*>(m1);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_lockread,1,((in),void*,m1))
{
	static_cast<OOBase::RWMutex*>(m1)->acquire_read();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_lockwrite,1,((in),void*,m1))
{
	static_cast<OOBase::RWMutex*>(m1)->acquire();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_unlockread,1,((in),void*,m1))
{
	static_cast<OOBase::RWMutex*>(m1)->release_read();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_unlockwrite,1,((in),void*,m1))
{
	static_cast<OOBase::RWMutex*>(m1)->release();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_atomic_addref,1,((in),size_t*,v))
{
	OOBase::Atomic<size_t>::Increment(*v);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_atomic_release,1,((in),size_t*,v))
{
	return (OOBase::Atomic<size_t>::Decrement(*v) == 0) ? 1 : 0;
}

namespace
{
	struct destruct_entry_t
	{
		Omega::Threading::DestructorCallback pfn_dctor;
		void*                                param;

		bool operator == (const destruct_entry_t& rhs) const
		{
			return (pfn_dctor == rhs.pfn_dctor && param == rhs.param);
		}
	};

	struct mod_destruct_t
	{
		OOBase::SpinLock                m_lock;
		OOBase::Stack<destruct_entry_t> m_list;
	};
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_mod_destruct__ctor,1,((in),void**,phandle))
{
	void* pCur = OOBase::Atomic<void*>::CompareAndSwap(*phandle,NULL,(void*)1);
	if (!pCur)
		*phandle = new (OOBase::critical) mod_destruct_t();

	while (pCur == (void*)1)
	{
		OOBase::Thread::yield();
		pCur = *phandle;
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_mod_destruct__dctor,1,((in),void*,handle))
{
	mod_destruct_t* h = static_cast<mod_destruct_t*>(handle);
	if (h)
	{
		OOBase::Guard<OOBase::SpinLock> guard(h->m_lock);

		destruct_entry_t e;
		while (h->m_list.pop(&e))
		{
			guard.release();

			try
			{
				(*e.pfn_dctor)(e.param);
			}
			catch (Omega::IException* pE)
			{
				pE->Release();
			}
			catch (...)
			{}

			guard.acquire();
		}
	}

	delete h;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_mod_destruct_add,3,((in),void*,handle,(in),Omega::Threading::DestructorCallback,pfn_dctor,(in),void*,param))
{
	mod_destruct_t* h = static_cast<mod_destruct_t*>(handle);
	if (h)
	{
		OOBase::Guard<OOBase::SpinLock> guard(h->m_lock);

		destruct_entry_t e = { pfn_dctor, param };

		int err = h->m_list.push(e);
		if (err != 0)
			OMEGA_THROW(err);
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_mod_destruct_remove,3,((in),void*,handle,(in),Omega::Threading::DestructorCallback,pfn_dctor,(in),void*,param))
{
	mod_destruct_t* h = static_cast<mod_destruct_t*>(handle);
	if (h)
	{
		OOBase::Guard<OOBase::SpinLock> guard(h->m_lock);

		destruct_entry_t e = { pfn_dctor, param };

		h->m_list.erase(e);
	}
}
