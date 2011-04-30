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

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_cs__ctor,0,());
inline Omega::Threading::Mutex::Mutex() :
		m_handle(static_cast<handle_t*>(OOCore_cs__ctor()))
{
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_cs__dctor,1,((in),void*,h));
inline Omega::Threading::Mutex::~Mutex()
{
	try
	{
		OOCore_cs__dctor(m_handle);
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_cs_lock,1,((in),void*,h));
inline void Omega::Threading::Mutex::Acquire()
{
	OOCore_cs_lock(m_handle);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_cs_unlock,1,((in),void*,h));
inline void Omega::Threading::Mutex::Release()
{
	OOCore_cs_unlock(m_handle);
}

OOCORE_RAW_EXPORTED_FUNCTION(void*,OOCore_rw_lock__ctor,0,());
inline Omega::Threading::ReaderWriterLock::ReaderWriterLock() :
		m_handle(static_cast<handle_t*>(OOCore_rw_lock__ctor()))
{
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock__dctor,1,((in),void*,h));
inline Omega::Threading::ReaderWriterLock::~ReaderWriterLock()
{
	try
	{
		OOCore_rw_lock__dctor(m_handle);
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_lockread,1,((in),void*,h));
inline void Omega::Threading::ReaderWriterLock::AcquireRead()
{
	OOCore_rw_lock_lockread(m_handle);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_lockwrite,1,((in),void*,h));
inline void Omega::Threading::ReaderWriterLock::Acquire()
{
	OOCore_rw_lock_lockwrite(m_handle);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_unlockread,1,((in),void*,h));
inline void Omega::Threading::ReaderWriterLock::ReleaseRead()
{
	OOCore_rw_lock_unlockread(m_handle);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_unlockwrite,1,((in),void*,h));
inline void Omega::Threading::ReaderWriterLock::Release()
{
	OOCore_rw_lock_unlockwrite(m_handle);
}

template <typename DLL>
inline void Omega::Threading::ModuleDestructor<DLL>::add_destructor(void (OMEGA_CALL *pfn_dctor)(void*), void* param)
{
	ModuleDestructor& inst = instance();
	Guard<Mutex> guard(inst.m_lock);
	inst.m_list.push_front(std::pair<void (OMEGA_CALL*)(void*),void*>(pfn_dctor,param));
}

template <typename DLL>
inline void Omega::Threading::ModuleDestructor<DLL>::remove_destructor(void (OMEGA_CALL *pfn_dctor)(void*), void* param)
{
	ModuleDestructor& inst = instance();
	Guard<Mutex> guard(inst.m_lock);

	for (std::list<std::pair<void (OMEGA_CALL*)(void*),void*> >::iterator i=inst.m_list.begin(); i!=inst.m_list.end(); ++i)
	{
		if (i->first == pfn_dctor && i->second == param)
		{
			inst.m_list.erase(i);
			break;
		}
	}
}

template <typename DLL>
inline Omega::Threading::ModuleDestructor<DLL>::~ModuleDestructor()
{
	Guard<Mutex> guard(m_lock);

	try
	{
		// Copy the list outside the lock
		std::list<std::pair<void (OMEGA_CALL*)(void*),void*> > list(m_list);

		m_list.clear();

		guard.Release();

		for (std::list<std::pair<void (OMEGA_CALL*)(void*),void*> >::iterator i=list.begin(); i!=list.end(); ++i)
		{
			try
			{
				(*(i->first))(i->second);
			}
			catch (IException* pE)
			{
				pE->Release();
			}
			catch (...)
			{}
		}
	}
	catch (std::exception&)
	{}
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_add_uninit_call,2,((in),void*,pfn_dctor,(in),void*,param));
OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_remove_uninit_call,2,((in),void*,pfn_dctor,(in),void*,param));

template <typename DLL>
inline void Omega::Threading::InitialiseDestructor<DLL>::add_destructor(void (OMEGA_CALL *pfn_dctor)(void*), void* param)
{
	multi_dctor* p = new multi_dctor(pfn_dctor,param);

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
inline void Omega::Threading::InitialiseDestructor<DLL>::destruct(void* param)
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
	catch (std::exception&)
	{}

	multi_dctor* p = static_cast<multi_dctor*>(param);

	try
	{
		// Now call the destructor
		(*(p->pfn_dctor))(p->param);
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}
	catch (...)
	{}

	delete p;
}

template <typename T, typename Lifetime>
void* Omega::Threading::Singleton<T,Lifetime>::s_instance = 0;

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_sngtn_once,2,((in),void**,val,(in),Omega::Threading::SingletonCallback,pfn_init));
template <typename T, typename Lifetime>
inline T* Omega::Threading::Singleton<T,Lifetime>::instance()
{
	OOCore_sngtn_once(&s_instance,&do_init);
	return static_cast<T*>(s_instance);
}

template <typename T, typename Lifetime>
inline const Omega::System::Internal::SafeShim* Omega::Threading::Singleton<T,Lifetime>::do_init()
{
	try
	{
		s_instance = new T();

		Lifetime::add_destructor(do_term,0);
		return 0;
	}
	catch (Omega::IException* pE)
	{
		return System::Internal::return_safe_exception(pE);
	}
	catch (std::exception& e)
	{
		return System::Internal::return_safe_exception(IInternalException::Create(e,"Omega::Threading::Singleton::constructor()"));
	}
	catch (...)
	{
		return System::Internal::return_safe_exception(IInternalException::Create("Unhandled exception","Omega::Threading::Singleton::constructor()"));
	}
}

template <typename T, typename Lifetime>
inline void Omega::Threading::Singleton<T,Lifetime>::do_term(void*)
{
	try
	{
		delete static_cast<T*>(s_instance);

		s_instance = 0;
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}
	catch (...)
	{}
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_atomic_addref,1,((in),size_t*,v));
inline void Omega::Threading::AtomicRefCount::AddRef()
{
	OOCore_atomic_addref(&m_value);
}

OOCORE_RAW_EXPORTED_FUNCTION(int,OOCore_atomic_release,1,((in),size_t*,v));
inline bool Omega::Threading::AtomicRefCount::Release()
{
	return (OOCore_atomic_release(&m_value) != 0);
}

#endif // OMEGA_THREADING_INL_INCLUDED_
