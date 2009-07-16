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

#ifndef OMEGA_THREADING_INL_INCLUDED_
#define OMEGA_THREADING_INL_INCLUDED_

OMEGA_EXPORTED_FUNCTION(void*,OOCore_cs__ctor,0,());
Omega::Threading::Mutex::Mutex() :
	m_handle(static_cast<handle_t*>(OOCore_cs__ctor()))
{
}

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_cs__dctor,1,((in),void*,h));
Omega::Threading::Mutex::~Mutex()
{
	OOCore_cs__dctor(m_handle);
}

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_cs_lock,1,((in),void*,h));
void Omega::Threading::Mutex::Acquire()
{
	OOCore_cs_lock(m_handle);
}

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_cs_unlock,1,((in),void*,h));
void Omega::Threading::Mutex::Release()
{
	OOCore_cs_unlock(m_handle);
}

OMEGA_EXPORTED_FUNCTION(void*,OOCore_rw_lock__ctor,0,());
Omega::Threading::ReaderWriterLock::ReaderWriterLock() :
	m_handle(static_cast<handle_t*>(OOCore_rw_lock__ctor()))
{	
}

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_rw_lock__dctor,1,((in),void*,h));
Omega::Threading::ReaderWriterLock::~ReaderWriterLock()
{
	OOCore_rw_lock__dctor(m_handle);
}

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_lockread,1,((in),void*,h));
void Omega::Threading::ReaderWriterLock::AcquireRead()
{
	OOCore_rw_lock_lockread(m_handle);
}

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_lockwrite,1,((in),void*,h));
void Omega::Threading::ReaderWriterLock::Acquire()
{
	OOCore_rw_lock_lockwrite(m_handle);
}

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_unlockread,1,((in),void*,h));
void Omega::Threading::ReaderWriterLock::ReleaseRead()
{
	OOCore_rw_lock_unlockread(m_handle);
}

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_unlockwrite,1,((in),void*,h));
void Omega::Threading::ReaderWriterLock::Release()
{
	OOCore_rw_lock_unlockwrite(m_handle);
}

template <typename DLL>
void Omega::Threading::ModuleDestructor<DLL>::add_destructor(pfn_destructor pfn, void* param)
{
	try
	{
		ModuleDestructor& inst = instance();
		Guard<Mutex> guard(inst.m_lock);
		inst.m_list.push_front(std::pair<pfn_destructor,void*>(pfn,param));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

template <typename DLL>
void Omega::Threading::ModuleDestructor<DLL>::remove_destructor(pfn_destructor pfn, void* param)
{
	try
	{
		ModuleDestructor& inst = instance();
		Guard<Mutex> guard(inst.m_lock);

		for (std::list<std::pair<pfn_destructor,void*> >::iterator i=inst.m_list.begin();i!=inst.m_list.end();++i)
		{
			if (i->first == pfn && i->second == param)
			{
				inst.m_list.erase(i);
				break;
			}
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

template <typename DLL>
Omega::Threading::ModuleDestructor<DLL>::~ModuleDestructor()
{
	try
	{
		Guard<Mutex> guard(m_lock);

		// Copy the list outside the lock
		std::list<std::pair<pfn_destructor,void*> > list(m_list);

		m_list.clear();

		guard.Release();
	
		for (std::list<std::pair<pfn_destructor,void*> >::iterator i=list.begin();i!=list.end();++i)
		{
			(*(i->first))(i->second);
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_add_uninit_call,2,((in),void*,pfn_dctor,(in),void*,param));
OMEGA_EXPORTED_FUNCTION_VOID(OOCore_remove_uninit_call,2,((in),void*,pfn_dctor,(in),void*,param));

template <typename DLL>
void Omega::Threading::InitialiseDestructor<DLL>::add_destructor(pfn_destructor pfn, void* param)
{
	multi_dctor* p = 0;
	OMEGA_NEW(p,multi_dctor);

	p->pfn = pfn;
	p->param = param;

	try
	{
		OOCore_add_uninit_call((void*)destruct,p);
	}
	catch (...)
	{
		delete p;
		throw;
	}

	try
	{
		ModuleDestructor<DLL>::add_destructor(destruct,p);
	}
	catch (...)
	{
		OOCore_remove_uninit_call((void*)destruct,p);
		delete p;
		throw;
	}
}

template <typename DLL>
void Omega::Threading::InitialiseDestructor<DLL>::destruct(void* param)
{
	try
	{
		OOCore_remove_uninit_call((void*)destruct,param);
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}
	catch (...)
	{}

	try
	{
		ModuleDestructor<DLL>::remove_destructor(destruct,param);
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}
	catch (...)
	{}

	multi_dctor* p = static_cast<multi_dctor*>(param);

	// Now call the destructor
	(*(p->pfn))(p->param);
	delete p;
}

template <typename T, typename Lifetime>
void* Omega::Threading::Singleton<T,Lifetime>::s_instance = 0;

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_sngtn_once,2,((in),void**,val,(in),void*,pfn_init));
template <typename T, typename Lifetime>
T* Omega::Threading::Singleton<T,Lifetime>::instance()
{
	OOCore_sngtn_once(&s_instance,(void*)&do_init);
	return static_cast<T*>(s_instance);
}

#ifdef OMEGA_DEBUG
#define OMEGA_DEBUG_STASH_ATOMIC(expr)	m_debug_value expr
#else
#define OMEGA_DEBUG_STASH_ATOMIC(expr)	(void)0
#endif

OMEGA_EXPORTED_FUNCTION(void*,OOCore_atomic__ctor,0,());
Omega::Threading::AtomicRefCount::AtomicRefCount() :
	m_handle(static_cast<handle_t*>(OOCore_atomic__ctor()))
{
	OMEGA_DEBUG_STASH_ATOMIC(=0);
}

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_atomic__dctor,1,((in),void*,h));
Omega::Threading::AtomicRefCount::~AtomicRefCount()
{
	OOCore_atomic__dctor(m_handle);
}

OMEGA_EXPORTED_FUNCTION(int,OOCore_atomic_addref,1,((in),void*,h));
bool Omega::Threading::AtomicRefCount::AddRef()
{
	OMEGA_DEBUG_STASH_ATOMIC(++);
	return (OOCore_atomic_addref(m_handle) != 0);
}

OMEGA_EXPORTED_FUNCTION(int,OOCore_atomic_release,1,((in),void*,h));
bool Omega::Threading::AtomicRefCount::Release()
{
	OMEGA_DEBUG_STASH_ATOMIC(--);
	return (OOCore_atomic_release(m_handle) != 0);
}

OMEGA_EXPORTED_FUNCTION(int,OOCore_atomic_iszero,1,((in),void*,h));
bool Omega::Threading::AtomicRefCount::IsZero() const
{
	return (OOCore_atomic_iszero(m_handle) != 0);
}

#endif // OMEGA_THREADING_INL_INCLUDED_
