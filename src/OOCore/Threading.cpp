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

using namespace Omega;
using namespace OTL;

template class OOBase::Singleton<OOBase::SpinLock,OOCore::DLL>;

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_sngtn_once,3,((in),void**,val,(in),size_t,n,(in),Threading::SingletonCallback,pfn_init))
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
			// Allocate
			*val = System::Allocate(n);

			// Call the init function
			const System::Internal::SafeShim* pE = (*pfn_init)(val);
			if (pE)
			{
				System::Free(*val);
				*val = NULL;
				System::Internal::throw_correct_exception(pE);
			}
		}
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_cs__ctor,0,())
{
	return new (OOCore::throwing) OOBase::Mutex();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_cs__dctor,1,((in),void*,m1))
{
	delete static_cast<OOBase::Mutex*>(m1);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_cs_lock,1,((in),void*,m1))
{
	if (!static_cast<OOBase::Mutex*>(m1)->try_acquire())
	{
		ObjectPtr<Remoting::ICallContext> ptrCC = Remoting::GetCallContext();
		uint32_t millisecs = ptrCC->Timeout();
		if (millisecs != 0xFFFFFFFF)
		{
			OOBase::Timeout timeout(millisecs / 1000,(millisecs % 1000) * 1000);
			if (!static_cast<OOBase::Mutex*>(m1)->acquire(timeout))
				throw ITimeoutException::Create();
		}
		else
		{
			static_cast<OOBase::Mutex*>(m1)->acquire();
		}
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_cs_unlock,1,((in),void*,m1))
{
	static_cast<OOBase::Mutex*>(m1)->release();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_atomic_addref,1,((in),size_t*,v))
{
	return OOBase::Atomic<size_t>::Increment(*v);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(size_t,OOCore_atomic_release,1,((in),size_t*,v))
{
	return OOBase::Atomic<size_t>::Decrement(*v);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_atomic_is_zero,1,((in),const size_t*,v))
{
	return (OOBase::Atomic<size_t>::CompareAndSwap(*const_cast<size_t*>(v),0,0) == 0 ? 1 : 0);
}

namespace
{
	struct destruct_entry_t
	{
		Threading::DestructorCallback pfn_dctor;
		void*                                param;

		bool operator == (const destruct_entry_t& rhs) const
		{
			return (pfn_dctor == rhs.pfn_dctor && param == rhs.param);
		}
	};

	struct mod_destruct_t
	{
		OOBase::SpinLock                m_lock;
		OOBase::Stack<destruct_entry_t> m_stack;
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

		for (destruct_entry_t e;h->m_stack.pop(&e);)
		{
			guard.release();

			try
			{
				(*e.pfn_dctor)(e.param);
			}
			catch (IException* pE)
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

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_mod_destruct_add,3,((in),void*,handle,(in),Threading::DestructorCallback,pfn_dctor,(in),void*,param))
{
	mod_destruct_t* h = static_cast<mod_destruct_t*>(handle);
	if (h)
	{
		OOBase::Guard<OOBase::SpinLock> guard(h->m_lock);

		destruct_entry_t e = { pfn_dctor, param };

		int err = h->m_stack.push(e);
		if (err != 0)
			OMEGA_THROW(err);
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_mod_destruct_remove,3,((in),void*,handle,(in),Threading::DestructorCallback,pfn_dctor,(in),void*,param))
{
	mod_destruct_t* h = static_cast<mod_destruct_t*>(handle);
	if (h)
	{
		OOBase::Guard<OOBase::SpinLock> guard(h->m_lock);

		destruct_entry_t e = { pfn_dctor, param };

		for (size_t pos = 0;pos < h->m_stack.size();++pos)
		{
			if (*h->m_stack.at(pos) == e)
			{
				h->m_stack.remove_at(pos);
				break;
			}
		}
	}
}

namespace
{
	class SingletonHolder
	{
	public:
		void close_singletons();
		void add_uninit_call(Threading::DestructorCallback pfn, void* param);
		void remove_uninit_call(Threading::DestructorCallback pfn, void* param);

	private:
		struct Uninit
		{
			Threading::DestructorCallback pfn_dctor;
			void*                                param;

			bool operator == (const Uninit& rhs) const
			{
				return (pfn_dctor == rhs.pfn_dctor && param == rhs.param);
			}
		};
		OOBase::SpinLock      m_lock;
		OOBase::Stack<Uninit> m_stackUninitCalls;
	};

	typedef OOBase::Singleton<SingletonHolder,OOCore::DLL> SINGLETON_HOLDER;
}
template class OOBase::Singleton<SingletonHolder,OOCore::DLL>;

namespace OOCore
{
	void CloseSingletons()
	{
		SINGLETON_HOLDER::instance().close_singletons();
	}
}

void SingletonHolder::close_singletons()
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	for (Uninit uninit;m_stackUninitCalls.pop(&uninit);)
	{
		guard.release();

		try
		{
			OOBase::Guard<OOBase::SpinLock> guard2(OOBase::Singleton<OOBase::SpinLock,OOCore::DLL>::instance());

			(*uninit.pfn_dctor)(uninit.param);
		}
		catch (IException* pE)
		{
			pE->Release();
		}
		catch (...)
		{}

		guard.acquire();
	}
}

void SingletonHolder::add_uninit_call(Threading::DestructorCallback pfn, void* param)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	Uninit uninit = { pfn, param };

	int err = m_stackUninitCalls.push(uninit);
	if (err != 0)
		OMEGA_THROW(err);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_add_uninit_call,2,((in),Omega::Threading::DestructorCallback,pfn_dctor,(in),void*,param))
{
	SINGLETON_HOLDER::instance().add_uninit_call(pfn_dctor,param);
}

void SingletonHolder::remove_uninit_call(Threading::DestructorCallback pfn, void* param)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	Uninit uninit = { pfn, param };

	for (size_t pos = 0;pos < m_stackUninitCalls.size();++pos)
	{
		if (*m_stackUninitCalls.at(pos) == uninit)
		{
			m_stackUninitCalls.remove_at(pos);
			break;
		}
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_remove_uninit_call,2,((in),Omega::Threading::DestructorCallback,pfn_dctor,(in),void*,param))
{
	SingletonHolder* p = SINGLETON_HOLDER::instance_ptr();
	if (p)
		p->remove_uninit_call(pfn_dctor,param);
}

