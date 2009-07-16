///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOBase, the Omega Online Base library.
//
// OOBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOBASE_ATOMIC_H_INCLUDED_
#define OOBASE_ATOMIC_H_INCLUDED_

#include "Mutex.h"

#if defined(HAVE___SYNC_TEST_AND_SET)

/* Define to if you have atomic exchange for 32bit values */
#define ATOMIC_EXCH_32(t,v) __sync_lock_test_and_set((long volatile*)(t),v)

/* Define to if you have atomic exchange for 64bit values */
#define ATOMIC_EXCH_64(t,v) __sync_lock_test_and_set((long long volatile*)(t),v)

#if defined(HAVE___SYNC_ADD_AND_FETCH)

/* Define to if you have atomic inc and dec for 32bit values */
#define ATOMIC_INC_32(t) __sync_add_and_fetch((long*)(t),1)
#define ATOMIC_DEC_32(t) __sync_sub_and_fetch((long*)(t),1)
#define ATOMIC_ADD_32(t,v) __sync_add_and_fetch((long volatile*)(t),v)
#define ATOMIC_SUB_32(t,v) __sync_sub_and_fetch((long volatile*)(t),v)

/* Define to if you have atomic exchange for 64bit values */
#define ATOMIC_INC_64(t) __sync_add_and_fetch((long long volatile*)(t),1)
#define ATOMIC_DEC_64(t) __sync_sub_and_fetch((long long volatile*)(t),1)
#define ATOMIC_ADD_64(t,v) __sync_add_and_fetch((long long volatile*)(t),v)
#define ATOMIC_SUB_64(t,v) __sync_sub_and_fetch((long long volatile*)(t),v)

#endif // defined(HAVE___SYNC_ADD_AND_FETCH)

#elif defined(_MSC_VER)

#include <intrin.h>

/* Define to if you have atomic exchange for 32bit values */
#define ATOMIC_EXCH_32(t,v) _InterlockedExchange((long volatile*)(t),(long)(v))

/* Define to if you have atomic inc and dec for 32bit values */
#define ATOMIC_INC_32(t) _InterlockedIncrement((long volatile*)(t))
#define ATOMIC_DEC_32(t) _InterlockedDecrement((long volatile*)(t))
#define ATOMIC_ADD_32(t,v) _InterlockedExchangeAdd((long volatile*)(t),(long)(v))
#define ATOMIC_SUB_32(t,v) _InterlockedExchangeAdd((long volatile*)(t),-(long)(v))

/* Define to if you have atomic exchange for 64bit values */
#define ATOMIC_EXCH_64(t,v) _InterlockedExchange64((__int64 volatile*)(t),(__int64)(v))

/* Define to if you have atomic exchange for 64bit values */
#define ATOMIC_INC_64(t) _InterlockedIncrement64((__int64 volatile*)(t))
#define ATOMIC_DEC_64(t) _InterlockedDecrement64((__int64 volatile*)(t))
#define ATOMIC_ADD_64(t,v) _InterlockedExchangeAdd64((__int64 volatile*)(t),(__int64)(v))
#define ATOMIC_SUB_64(t,v) _InterlockedExchangeAdd64((__int64 volatile*)(t),-(__int64)(v))

#elif defined(_WIN32)

/* Define to if you have atomic exchange for 32bit values */
#define ATOMIC_EXCH_32(t,v) InterlockedExchange((LONG volatile*)(t),(LONG)(v))

/* Define to if you have atomic inc and dec for 32bit values */
#define ATOMIC_INC_32(t) InterlockedIncrement((LONG volatile*)(t))
#define ATOMIC_DEC_32(t) InterlockedDecrement((LONG volatile*)(t))
#define ATOMIC_ADD_32(t,v) InterlockedExchangeAdd((LONG volatile*)(t),(LONG)(v))
#define ATOMIC_SUB_32(t,v) InterlockedExchangeAdd((LONG volatile*)(t),-(LONG)(v))

#if (WINVER >= 0x0501)
/* Define to if you have atomic exchange for 64bit values */
#define ATOMIC_EXCH_64(t,v) InterlockedExchange64((LONGLONG volatile*)(t),(LONGLONG)(v))

/* Define to if you have atomic exchange for 64bit values */
#define ATOMIC_INC_64(t) InterlockedIncrement64((LONGLONG volatile*)(t))
#define ATOMIC_DEC_64(t) InterlockedDecrement64((LONGLONG volatile*)(t))
#define ATOMIC_ADD_64(t,v) InterlockedExchangeAdd64((LONGLONG volatile*)(t),(LONGLONG)(v))
#define ATOMIC_SUB_64(t,v) InterlockedExchangeAdd64((LONGLONG volatile*)(t),-(LONGLONG)(v))

#endif // WINVER >= 0x0502

#endif // _WIN32

namespace OOBase
{
	namespace detail
	{
		template <typename T>
		class AtomicValImpl_Raw
		{
		public:
			AtomicValImpl_Raw(const T& v) : m_val(v) {}
			AtomicValImpl_Raw(const AtomicValImpl_Raw& rhs) : m_val(rhs.value()) {}

			AtomicValImpl_Raw& operator = (const T& v)
			{
				Guard<SpinLock> guard(m_lock);
				m_val = v;
				return *this;
			}

			AtomicValImpl_Raw& operator = (const AtomicValImpl_Raw& rhs)
			{
				if (this != &rhs)
				{
					Guard<SpinLock> guard(m_lock);
					m_val = rhs.value();
				}
				
				return *this;
			}

			bool operator == (const AtomicValImpl_Raw& rhs) const
			{
				return (value() == rhs.value());
			}

			bool operator != (const AtomicValImpl_Raw& rhs) const
			{
				return (value() != rhs.value());
			}

			bool operator == (const T& v) const
			{
				return (value() == v);
			}

			bool operator != (const T& v) const
			{
				return (value() != v);
			}

			T value() const
			{
				Guard<SpinLock> guard(m_lock);
				return m_val;
			}
			
		protected:
			AtomicValImpl_Raw() {}

			mutable SpinLock m_lock;
			T                m_val;	
		};

		template <typename T, const size_t S>
		class AtomicValImpl : public AtomicValImpl_Raw<T>
		{
		public:
			AtomicValImpl(T v) : AtomicValImpl_Raw<T>(v) {}
			AtomicValImpl(const T& v) : AtomicValImpl_Raw<T>(v) {}
			AtomicValImpl(const AtomicValImpl& rhs) : AtomicValImpl_Raw<T>(rhs.value()) {}
			
			AtomicValImpl& operator = (const AtomicValImpl& rhs)
			{
				if (this != &rhs)
					AtomicValImpl_Raw<T>::operator = (rhs.value());
					
				return *this;
			}

		private:
			AtomicValImpl();
		};

		template <typename T, const size_t S>
		class AtomicIntImpl : public AtomicValImpl_Raw<T>
		{
		public:
			AtomicIntImpl() : AtomicValImpl_Raw<T>(T(0)) {}
			AtomicIntImpl(const T& v) : AtomicValImpl_Raw<T>(v) {}
			AtomicIntImpl(const AtomicIntImpl& rhs) : AtomicValImpl_Raw<T>(rhs.value()) {}
			
			AtomicIntImpl& operator = (const AtomicIntImpl& rhs)
			{
				if (this != &rhs)
					AtomicValImpl_Raw<T>::operator = (rhs.value());
					
				return *this;
			}

			AtomicIntImpl& operator += (const T& v)
			{
				Guard<SpinLock> guard(this->m_lock);
				this->m_val += v;
				return *this;
			}

			AtomicIntImpl& operator -= (const T& v)
			{
				Guard<SpinLock> guard(this->m_lock);
				this->m_val -= v;
				return *this;
			}

			T operator ++()
			{
				Guard<SpinLock> guard(this->m_lock);
				return ++this->m_val;
			}

			T operator --()
			{
				Guard<SpinLock> guard(this->m_lock);
				return --this->m_val;
			}
		};
	}

	template <typename T>
	class AtomicVal : public detail::AtomicValImpl<T,sizeof(T)>
	{
	public:
		AtomicVal(const T& v) : detail::AtomicValImpl<T,sizeof(T)>(v) {}
		AtomicVal(const AtomicVal& rhs) : detail::AtomicValImpl<T,sizeof(T)>(rhs.value()) {}

		AtomicVal& operator = (const AtomicVal& rhs)
		{
			if (this != &rhs)
				detail::AtomicValImpl<T,sizeof(T)>::operator = (rhs.value());
				
			return *this;
		}

	private:
		AtomicVal();
	};

	template <typename T>
	class AtomicInt : public detail::AtomicIntImpl<T,sizeof(T)>
	{
	public:
		AtomicInt() : detail::AtomicIntImpl<T,sizeof(T)>() {}
		AtomicInt(const T& v) : detail::AtomicIntImpl<T,sizeof(T)>(v) {}
		AtomicInt(const AtomicVal<T>& rhs) : detail::AtomicIntImpl<T,sizeof(T)>(rhs.value()) {}
		AtomicInt(const AtomicInt& rhs) : detail::AtomicIntImpl<T,sizeof(T)>(rhs.value()) {}

		AtomicInt& operator = (const AtomicInt& rhs)
		{
			if (this != &rhs)
				detail::AtomicIntImpl<T,sizeof(T)>::operator = (rhs.value());
				
			return *this;
		}

		T operator ++()
		{
			return detail::AtomicIntImpl<T,sizeof(T)>::operator ++();
		}

		T operator --()
		{
			return detail::AtomicIntImpl<T,sizeof(T)>::operator --();
		}

		T operator ++(int) 
		{ 
			return ++*this - 1; 
		}

		T operator --(int) 
		{
			return --*this + 1; 
		}
	};

#if defined(ATOMIC_EXCH_32)
	namespace detail
	{
		template <typename T>
		class AtomicValImpl<T,4>
		{
		public:
			AtomicValImpl(const T& v) : m_val(v) {}
			AtomicValImpl(const AtomicValImpl& rhs) : m_val(rhs.value()) {}

			AtomicValImpl& operator = (const T& v)
			{
				ATOMIC_EXCH_32(&m_val,v);
				return *this;
			}

			AtomicValImpl& operator = (const AtomicValImpl& rhs)
			{
				if (this != &rhs)
					ATOMIC_EXCH_32(&m_val,rhs.m_val);
				
				return *this;
			}

			bool operator == (const AtomicValImpl& rhs) const
			{
				return (m_val == rhs.m_val);
			}

			bool operator != (const AtomicValImpl& rhs) const
			{
				return (m_val != rhs.m_val);
			}

			bool operator == (const T& v) const
			{
				return (m_val == v);
			}

			bool operator != (const T& v) const
			{
				return (m_val != v);
			}

			T value() const
			{
				return m_val;
			}
			
		protected:
			AtomicValImpl() {}

			volatile T m_val;			
		};
	}
#endif // ATOMIC_EXCH_32

#if defined(ATOMIC_EXCH_64)
	namespace detail
	{
		template <typename T>
		class AtomicValImpl<T,8>
		{
		public:
			AtomicValImpl(const T& v) : m_val(v) {}
			AtomicValImpl(const AtomicValImpl& rhs) : m_val(rhs.value()) {}

			AtomicValImpl& operator = (const T& v)
			{
				ATOMIC_EXCH_64(&m_val,v);
				return *this;
			}

			AtomicValImpl& operator = (const AtomicValImpl& rhs)
			{
				if (this != &rhs)
					ATOMIC_EXCH_64(&m_val,rhs.m_val);
				
				return *this;
			}

			bool operator == (const AtomicValImpl& rhs) const
			{
				return (m_val == rhs.m_val);
			}

			bool operator != (const AtomicValImpl& rhs) const
			{
				return (m_val != rhs.m_val);
			}

			bool operator == (const T& v) const
			{
				return (m_val == v);
			}

			bool operator != (const T& v) const
			{
				return (m_val != v);
			}

			T value() const
			{
				return m_val;
			}
			
		protected:
			AtomicValImpl();

			volatile T m_val;
		};
	}
#endif // ATOMIC_EXCH_64

#if defined(ATOMIC_INC_32)
	namespace detail
	{
		template <typename T>
		class AtomicIntImpl<T,4> : public AtomicValImpl<T,4>
		{
		public:
			AtomicIntImpl() : AtomicValImpl<T,4>(0) {}
			AtomicIntImpl(const T& v) : AtomicValImpl<T,4>(v) {}
			AtomicIntImpl(const AtomicIntImpl& rhs) : AtomicValImpl<T,4>(rhs.value()) {}
			
			AtomicIntImpl& operator = (const AtomicIntImpl& rhs)
			{
				if (this != &rhs)
					AtomicValImpl<T,4>::operator = (rhs.value());
					
				return *this;
			}

			AtomicIntImpl& operator += (const T& v)
			{
				ATOMIC_ADD_32(&this->m_val,v);
				return *this;
			}

			AtomicIntImpl& operator -= (const T& v)
			{
				ATOMIC_SUB_32(&this->m_val,v);
				return *this;
			}

			T operator ++()
			{
				return ATOMIC_INC_32(&this->m_val);
			}

			T operator --()
			{
				return ATOMIC_DEC_32(&this->m_val);
			}
		};
	}
#endif // defined(ATOMIC_INC_32)

#if defined(ATOMIC_INC_64)
	namespace detail
	{
		template <typename T>
		class AtomicIntImpl<T,8> : public AtomicValImpl<T,8>
		{
		public:
			AtomicIntImpl() : AtomicValImpl<T,8>(0) {}
			AtomicIntImpl(const T& v) : AtomicValImpl<T,8>(v) {}
			AtomicIntImpl(const AtomicIntImpl& rhs) : AtomicValImpl<T,8>(rhs.value()) {}
			
			AtomicIntImpl& operator = (const AtomicIntImpl& rhs)
			{
				if (this != &rhs)
					AtomicValImpl<T,8>::operator = (rhs.value());
					
				return *this;
			}

			AtomicIntImpl& operator += (const T& v)
			{
				ATOMIC_ADD_64(&this->m_val,v);
				return *this;
			}

			AtomicIntImpl& operator -= (const T& v)
			{
				ATOMIC_SUB_64(&this->m_val,v);
				return *this;
			}

			T operator ++()
			{
				return ATOMIC_INC_64(&this->m_val);
			}

			T operator --()
			{
				return ATOMIC_DEC_64(&this->m_val);
			}
		};
	}
#endif // defined(ATOMIC_INC_64)
}

#endif // OOBASE_ATOMIC_H_INCLUDED_
