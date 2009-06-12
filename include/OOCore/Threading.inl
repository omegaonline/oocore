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

template <typename T>
void* Omega::Threading::Singleton<T>::s_instance = 0;

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_sngtn_once,3,((in),void**,val,(in),void*,pfn_init,(in),void*,pfn_term));
template <typename T>
T* Omega::Threading::Singleton<T>::instance()
{
	OOCore_sngtn_once(&s_instance,(void*)&do_init,(void*)&do_term);
	return static_cast<T*>(s_instance);
}

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

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_atomic_addref,1,((in),void*,h));
void Omega::Threading::AtomicRefCount::AddRef()
{
	OOCore_atomic_addref(m_handle);
	OMEGA_DEBUG_STASH_ATOMIC(++);
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
