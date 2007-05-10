#ifndef OMEGA_THREADING_INL_INCLUDED_
#define OMEGA_THREADING_INL_INCLUDED_

template <class T>
Omega::AtomicOp<T>::AtomicOp(const T& v) :
	m_value(v)
{
}

template <class T>
Omega::AtomicOp<T>::AtomicOp(const AtomicOp& rhs) :
	m_value(rhs.value())
{
}

template <class T>
T Omega::AtomicOp<T>::operator ++()
{
	Guard guard(m_cs);
	return ++m_value;
}

template <class T>
T Omega::AtomicOp<T>::operator ++(int)
{
	return ++*this - 1;
}

template <class T>
T Omega::AtomicOp<T>::operator --()
{
	Guard guard(m_cs);
	return --m_value;
}

template <class T>
T Omega::AtomicOp<T>::operator --(int)
{
	return --*this + 1;
}

template <class T>
volatile T* Omega::AtomicOp<T>::operator &()
{
	return &m_value;
}

template <class T>
Omega::AtomicOp<T>& Omega::AtomicOp<T>::operator = (const AtomicOp& rhs)
{
	if (&rhs != this)
	{
		Guard guard(m_cs);
		m_value = rhs.value();
	}
	return *this;
}

template <class T>
Omega::AtomicOp<T>& Omega::AtomicOp<T>::operator = (const T& rhs)
{
	Guard guard(m_cs);
	m_value = rhs;
	return *this;
}

template <class T>
T Omega::AtomicOp<T>::value() const
{
	Guard guard(m_cs);
	return m_value;
}

template <class T>
volatile T& Omega::AtomicOp<T>::value()
{
	return m_value;
}

template <class T>
T Omega::AtomicOp<T>::exchange(const T& v)
{
	Guard guard(m_cs);
	T ret = m_value;
	m_value = v;
	return ret;
}

#ifdef OMEGA_HAS_ATOMIC_OP

Omega::AtomicOp<Omega::int32_t>::AtomicOp(const int32_t& v) :
	m_value(v)
{
}

Omega::AtomicOp<Omega::int32_t>::AtomicOp(const AtomicOp& rhs) :
	m_value(rhs.m_value)
{
}

Omega::AtomicOp<Omega::int32_t>& Omega::AtomicOp<Omega::int32_t>::operator = (const AtomicOp& rhs)
{
	OMEGA_ATOMIC_OP_EXCHANGE(&m_value,rhs.m_value);
	return *this;
}

Omega::AtomicOp<Omega::int32_t>& Omega::AtomicOp<Omega::int32_t>::operator = (const int32_t& rhs)
{
	OMEGA_ATOMIC_OP_EXCHANGE(&m_value,rhs);
	return *this;
}

Omega::int32_t Omega::AtomicOp<Omega::int32_t>::operator ++()
{
	return OMEGA_ATOMIC_OP_INCREMENT(&m_value);
}

Omega::int32_t Omega::AtomicOp<Omega::int32_t>::operator --()
{
	return OMEGA_ATOMIC_OP_DECREMENT(&m_value);
}

Omega::int32_t Omega::AtomicOp<Omega::int32_t>::exchange(const int32_t& v)
{
	return OMEGA_ATOMIC_OP_EXCHANGE(&m_value,v);
}

Omega::AtomicOp<Omega::uint32_t>::AtomicOp(const uint32_t& v) :
	m_value(v)
{
}

Omega::AtomicOp<Omega::uint32_t>::AtomicOp(const AtomicOp& rhs) :
	m_value(rhs.m_value)
{
}

Omega::AtomicOp<Omega::uint32_t>& Omega::AtomicOp<Omega::uint32_t>::operator = (const AtomicOp& rhs)
{
	OMEGA_ATOMIC_OP_EXCHANGE(&m_value,rhs.m_value);
	return *this;
}

Omega::AtomicOp<Omega::uint32_t>& Omega::AtomicOp<Omega::uint32_t>::operator = (const uint32_t& rhs)
{
	OMEGA_ATOMIC_OP_EXCHANGE(&m_value,rhs);
	return *this;
}

Omega::uint32_t Omega::AtomicOp<Omega::uint32_t>::operator ++()
{
	return OMEGA_ATOMIC_OP_INCREMENT(&m_value);
}

Omega::uint32_t Omega::AtomicOp<Omega::uint32_t>::operator --()
{
	return OMEGA_ATOMIC_OP_DECREMENT(&m_value);
}

Omega::uint32_t Omega::AtomicOp<Omega::uint32_t>::exchange(const uint32_t& v)
{
	return OMEGA_ATOMIC_OP_EXCHANGE(&m_value,v);
}

#endif // OMEGA_HAS_ATOMIC_OP

OOCORE_EXPORTED_FUNCTION(void*,cs__ctor,0,());
Omega::CriticalSection::CriticalSection() :
	m_handle(static_cast<handle_t>(cs__ctor()))
{
}

OOCORE_EXPORTED_FUNCTION_VOID(cs__dctor,1,((in),void*,h));
Omega::CriticalSection::~CriticalSection()
{
	cs__dctor(m_handle);
}

OOCORE_EXPORTED_FUNCTION_VOID(cs_lock,1,((in),void*,h));
void Omega::CriticalSection::Lock()
{
	cs_lock(m_handle);
}

OOCORE_EXPORTED_FUNCTION_VOID(cs_unlock,1,((in),void*,h));
void Omega::CriticalSection::Unlock()
{
	cs_unlock(m_handle);
}


OOCORE_EXPORTED_FUNCTION(void*,rw_lock__ctor,0,());
Omega::ReaderWriterLock::ReaderWriterLock() :
	m_handle(static_cast<handle_t>(rw_lock__ctor()))
{	
}

OOCORE_EXPORTED_FUNCTION_VOID(rw_lock__dctor,1,((in),void*,h));
Omega::ReaderWriterLock::~ReaderWriterLock()
{
	rw_lock__dctor(m_handle);
}

OOCORE_EXPORTED_FUNCTION_VOID(rw_lock_lockread,1,((in),void*,h));
void Omega::ReaderWriterLock::LockRead()
{
	rw_lock_lockread(m_handle);
}

OOCORE_EXPORTED_FUNCTION_VOID(rw_lock_lockwrite,1,((in),void*,h));
void Omega::ReaderWriterLock::LockWrite()
{
	rw_lock_lockwrite(m_handle);
}

OOCORE_EXPORTED_FUNCTION_VOID(rw_lock_unlock,1,((in),void*,h));
void Omega::ReaderWriterLock::Unlock()
{
	rw_lock_unlock(m_handle);
}

#endif // OMEGA_THREADING_INL_INCLUDED_
