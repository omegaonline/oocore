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

template <class T>
Omega::Threading::AtomicOp<T>::AtomicOp(const T& v) :
	m_value(v)
{
}

template <class T>
Omega::Threading::AtomicOp<T>::AtomicOp(const AtomicOp& rhs) :
	m_value(rhs.value())
{
}

template <class T>
T Omega::Threading::AtomicOp<T>::operator ++()
{
	Guard guard(m_cs);
	return ++m_value;
}

template <class T>
T Omega::Threading::AtomicOp<T>::operator ++(int)
{
	return ++*this - 1;
}

template <class T>
T Omega::Threading::AtomicOp<T>::operator --()
{
	Guard guard(m_cs);
	return --m_value;
}

template <class T>
T Omega::Threading::AtomicOp<T>::operator --(int)
{
	return --*this + 1;
}

template <class T>
volatile T* Omega::Threading::AtomicOp<T>::operator &()
{
	return &m_value;
}

template <class T>
Omega::Threading::AtomicOp<T>& Omega::Threading::AtomicOp<T>::operator = (const AtomicOp& rhs)
{
	if (&rhs != this)
	{
		Guard guard(m_cs);
		m_value = rhs.value();
	}
	return *this;
}

template <class T>
Omega::Threading::AtomicOp<T>& Omega::Threading::AtomicOp<T>::operator = (const T& rhs)
{
	Guard guard(m_cs);
	m_value = rhs;
	return *this;
}

template <class T>
bool Omega::Threading::AtomicOp<T>::operator == (const AtomicOp& rhs)
{
	Guard guard(m_cs);
	return m_value == rhs.value();
}

template <class T>
bool Omega::Threading::AtomicOp<T>::operator == (const T& rhs)
{
	Guard guard(m_cs);
	return m_value == rhs;
}

template <class T>
T Omega::Threading::AtomicOp<T>::value() const
{
	Guard guard(m_cs);
	return m_value;
}

template <class T>
T Omega::Threading::AtomicOp<T>::exchange(const T& v)
{
	Guard guard(m_cs);
	T ret = m_value;
	m_value = v;
	return ret;
}

#ifdef OMEGA_HAS_ATOMIC_OP_32

Omega::Threading::AtomicOp<Omega::int32_t>::AtomicOp(int32_t v) :
	m_value(v)
{
}

Omega::Threading::AtomicOp<Omega::int32_t>::AtomicOp(const AtomicOp& rhs) :
	m_value(rhs.m_value)
{
}

Omega::Threading::AtomicOp<Omega::int32_t>& Omega::Threading::AtomicOp<Omega::int32_t>::operator = (const AtomicOp& rhs)
{
	OMEGA_ATOMIC_OP_EXCHANGE_32(&m_value,rhs.m_value);
	return *this;
}

Omega::Threading::AtomicOp<Omega::int32_t>& Omega::Threading::AtomicOp<Omega::int32_t>::operator = (int32_t rhs)
{
	OMEGA_ATOMIC_OP_EXCHANGE_32(&m_value,rhs);
	return *this;
}

Omega::int32_t Omega::Threading::AtomicOp<Omega::int32_t>::operator ++()
{
	return OMEGA_ATOMIC_OP_INCREMENT_32(&m_value);
}

Omega::int32_t Omega::Threading::AtomicOp<Omega::int32_t>::operator --()
{
	return OMEGA_ATOMIC_OP_DECREMENT_32(&m_value);
}

Omega::int32_t Omega::Threading::AtomicOp<Omega::int32_t>::exchange(int32_t v)
{
	return static_cast<int32_t>(OMEGA_ATOMIC_OP_EXCHANGE_32(&m_value,v));
}

Omega::Threading::AtomicOp<Omega::uint32_t>::AtomicOp(uint32_t v) :
	m_value(v)
{
}

Omega::Threading::AtomicOp<Omega::uint32_t>::AtomicOp(const AtomicOp& rhs) :
	m_value(rhs.m_value)
{
}

Omega::Threading::AtomicOp<Omega::uint32_t>& Omega::Threading::AtomicOp<Omega::uint32_t>::operator = (const AtomicOp& rhs)
{
	OMEGA_ATOMIC_OP_EXCHANGE_32(&m_value,rhs.m_value);
	return *this;
}

Omega::Threading::AtomicOp<Omega::uint32_t>& Omega::Threading::AtomicOp<Omega::uint32_t>::operator = (uint32_t rhs)
{
	OMEGA_ATOMIC_OP_EXCHANGE_32(&m_value,rhs);
	return *this;
}

Omega::uint32_t Omega::Threading::AtomicOp<Omega::uint32_t>::operator ++()
{
	return OMEGA_ATOMIC_OP_INCREMENT_32(&m_value);
}

Omega::uint32_t Omega::Threading::AtomicOp<Omega::uint32_t>::operator --()
{
	return OMEGA_ATOMIC_OP_DECREMENT_32(&m_value);
}

Omega::uint32_t Omega::Threading::AtomicOp<Omega::uint32_t>::exchange(uint32_t v)
{
	return static_cast<uint32_t>(OMEGA_ATOMIC_OP_EXCHANGE_32(&m_value,v));
}

#if !defined(OMEGA_64)

template <class T>
Omega::Threading::AtomicOp<T*>::AtomicOp(T* v) :
	m_value(v)
{
}

template <class T>
Omega::Threading::AtomicOp<T*>::AtomicOp(const AtomicOp& rhs) :
	m_value(rhs.m_value)
{
}

template <class T>
Omega::Threading::AtomicOp<T*>& Omega::Threading::AtomicOp<T*>::operator = (const AtomicOp& rhs)
{
	OMEGA_ATOMIC_OP_EXCHANGE_32(&m_value,rhs.m_value);
	return *this;
}

template <class T>
Omega::Threading::AtomicOp<T*>& Omega::Threading::AtomicOp<T*>::operator = (T* rhs)
{
	OMEGA_ATOMIC_OP_EXCHANGE_32(&m_value,rhs);
	return *this;
}

template <class T>
T* Omega::Threading::AtomicOp<T*>::exchange(T* v)
{
	return reinterpret_cast<T*>(OMEGA_ATOMIC_OP_EXCHANGE_32(&m_value,v));
}
#endif // !defined(OMEGA_64)

#endif // OMEGA_HAS_ATOMIC_OP_32

OMEGA_EXPORTED_FUNCTION(void*,cs__ctor,0,());
Omega::Threading::CriticalSection::CriticalSection() :
	m_handle(static_cast<handle_t*>(cs__ctor()))
{
}

OMEGA_EXPORTED_FUNCTION_VOID(cs__dctor,1,((in),void*,h));
Omega::Threading::CriticalSection::~CriticalSection()
{
	cs__dctor(m_handle);
}

OMEGA_EXPORTED_FUNCTION_VOID(cs_lock,1,((in),void*,h));
void Omega::Threading::CriticalSection::Lock()
{
	cs_lock(m_handle);
}

OMEGA_EXPORTED_FUNCTION_VOID(cs_unlock,1,((in),void*,h));
void Omega::Threading::CriticalSection::Unlock()
{
	cs_unlock(m_handle);
}


OMEGA_EXPORTED_FUNCTION(void*,rw_lock__ctor,0,());
Omega::Threading::ReaderWriterLock::ReaderWriterLock() :
	m_handle(static_cast<handle_t*>(rw_lock__ctor()))
{	
}

OMEGA_EXPORTED_FUNCTION_VOID(rw_lock__dctor,1,((in),void*,h));
Omega::Threading::ReaderWriterLock::~ReaderWriterLock()
{
	rw_lock__dctor(m_handle);
}

OMEGA_EXPORTED_FUNCTION_VOID(rw_lock_lockread,1,((in),void*,h));
void Omega::Threading::ReaderWriterLock::LockRead()
{
	rw_lock_lockread(m_handle);
}

OMEGA_EXPORTED_FUNCTION_VOID(rw_lock_lockwrite,1,((in),void*,h));
void Omega::Threading::ReaderWriterLock::LockWrite()
{
	rw_lock_lockwrite(m_handle);
}

OMEGA_EXPORTED_FUNCTION_VOID(rw_lock_unlock,1,((in),void*,h));
void Omega::Threading::ReaderWriterLock::Unlock()
{
	rw_lock_unlock(m_handle);
}

#endif // OMEGA_THREADING_INL_INCLUDED_
