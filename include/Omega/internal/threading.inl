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

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_mod_destruct__ctor,1,((in),void**,phandle));
OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_mod_destruct__dctor,1,((in),void*,handle));

template <typename DLL>
inline void* Omega::Threading::ModuleDestructor<DLL>::handle()
{
	static void* s_handle = NULL;
	static auto_destructor s_i(s_handle);
	
	OOCore_mod_destruct__ctor(&s_handle);
	return s_handle;
}

template <typename DLL>
inline Omega::Threading::ModuleDestructor<DLL>::auto_destructor::~auto_destructor()
{
	OOCore_mod_destruct__dctor(m_h);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_mod_destruct_add,3,((in),void*,handle,(in),Omega::Threading::DestructorCallback,pfn_dctor,(in),void*,param));
OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_mod_destruct_remove,3,((in),void*,handle,(in),Omega::Threading::DestructorCallback,pfn_dctor,(in),void*,param));

template <typename DLL>
inline void Omega::Threading::ModuleDestructor<DLL>::add_destructor(DestructorCallback pfn, void* param)
{
	OOCore_mod_destruct_add(handle(),pfn,param);
}

template <typename DLL>
inline void Omega::Threading::ModuleDestructor<DLL>::remove_destructor(DestructorCallback pfn, void* param)
{
	OOCore_mod_destruct_remove(handle(),pfn,param);
}

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_add_uninit_call,2,((in),Omega::Threading::DestructorCallback,pfn_dctor,(in),void*,param));
OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_remove_uninit_call,2,((in),Omega::Threading::DestructorCallback,pfn_dctor,(in),void*,param));

template <typename DLL>
inline void Omega::Threading::InitialiseDestructor<DLL>::add_destructor(DestructorCallback pfn, void* param)
{
	multi_dctor* p = new (std::nothrow) multi_dctor(pfn,param);
	if (!p)
	{
#if defined(_WIN32)
		OMEGA_THROW(ERROR_OUTOFMEMORY);
#else
		OMEGA_THROW(ENOMEM);
#endif
	}

	try
	{
		OOCore_add_uninit_call(destruct,p);
		ModuleDestructor<DLL>::add_destructor(destruct,p);
	}
	catch (...)
	{
		OOCore_remove_uninit_call(destruct,p);
		delete p;
		throw;
	}
}

template <typename DLL>
inline void Omega::Threading::InitialiseDestructor<DLL>::destruct(void* param)
{
	OOCore_remove_uninit_call(destruct,param);
	
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

OOCORE_RAW_EXPORTED_FUNCTION_VOID(OOCore_sngtn_once,2,((in),void**,val,(in),Omega::Threading::SingletonCallback,pfn_init));
template <typename T, typename Lifetime>
inline T* Omega::Threading::Singleton<T,Lifetime>::instance()
{
	static void* instance = NULL;
	OOCore_sngtn_once(&instance,&do_init);
	return static_cast<T*>(instance);
}

template <typename T, typename Lifetime>
inline const Omega::System::Internal::SafeShim* Omega::Threading::Singleton<T,Lifetime>::do_init(void** param)
{
	try
	{
		*param = new T();

		Lifetime::add_destructor(do_term,param);
		return 0;
	}
	catch (Omega::IException* pE)
	{
		return System::Internal::return_safe_exception(pE);
	}
	catch (...)
	{
		return System::Internal::return_safe_exception(IInternalException::Create("Unhandled exception","Omega::Threading::Singleton::constructor()"));
	}
}

template <typename T, typename Lifetime>
inline void Omega::Threading::Singleton<T,Lifetime>::do_term(void* param)
{
	try
	{
		T** p = static_cast<T**>(param);
		delete *p;
		*p = NULL;
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
	}
	catch (...)
	{}
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_atomic_addref,1,((in),size_t*,v));
inline size_t Omega::Threading::AtomicRefCount::AddRef()
{
	return OOCore_atomic_addref(&m_value);
}

OOCORE_RAW_EXPORTED_FUNCTION(size_t,OOCore_atomic_release,1,((in),size_t*,v));
inline size_t Omega::Threading::AtomicRefCount::Release()
{
	return OOCore_atomic_release(&m_value);
}

OOCORE_RAW_EXPORTED_FUNCTION(int,OOCore_atomic_is_zero,1,((in),size_t*,v));
inline bool Omega::Threading::AtomicRefCount::IsZero() const
{
	return (OOCore_atomic_is_zero(const_cast<size_t*>(&m_value)) != 0);
}

#endif // OMEGA_THREADING_INL_INCLUDED_
