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

#ifndef OOBASE_MUTEX_H_INCLUDED_
#define OOBASE_MUTEX_H_INCLUDED_

#include "TimeVal.h"
#include "Win32.h"

namespace OOBase
{
	/// A recursive mutex that can be acquired with a timeout
	class Mutex
	{
	    friend class Condition;

	public:
		Mutex();
		~Mutex();

		bool tryacquire();
		bool acquire(const timeval_t* wait = 0);
		void release();

	private:
		// No copying
		Mutex(const Mutex&) {};
		Mutex& operator = (const Mutex&) { return *this; }

#if defined(_WIN32)
		Win32::SmartHandle m_mutex;
#elif defined(HAVE_PTHREAD)
		pthread_mutex_t    m_mutex;
#else
#error Fix me!
#endif
	};

#if defined(_WIN32) || defined(HAVE_FUTEX_H)
	/// A non-recursive mutex that spins in user-mode before aquiring the kernel mutex
	class SpinLock
	{
	public:
		SpinLock(unsigned int max_spin = 4001);
		~SpinLock();

		bool tryacquire();
		void acquire();
		void release();

	private:
		// No copying
		SpinLock(const SpinLock&) {};
		SpinLock& operator = (const SpinLock&) { return *this; }

#if defined(_WIN32)
		CRITICAL_SECTION m_cs;
#else
#error Futexes?!?
#endif
	};
#else
	typedef Mutex SpinLock;
#endif

	class RWMutex
	{
	public:
		RWMutex();
		~RWMutex();

		// Write lock
		void acquire();
		void release();

		// Read lock
		void acquire_read();
		void release_read();

	private:
		// No copying
		RWMutex(const RWMutex&) {};
		RWMutex& operator = (const RWMutex&) { return *this; }

#if defined(_WIN32)
		SRWLOCK          m_lock;
#elif defined(HAVE_PTHREAD)
		pthread_rwlock_t m_mutex;
#else
#error Fix me!
#endif
	};

	template <typename MUTEX>
	class Guard
	{
	public:
		Guard(MUTEX& mutex, bool acq = true) :
		  m_acquired(false),
		  m_mutex(mutex)
		{
			if (acq)
				acquire();
		}

		Guard(MUTEX& mutex, const timeval_t& wait) :
		  m_acquired(false),
		  m_mutex(mutex)
		{
			acquire(&wait);
		}

		~Guard()
		{
			if (m_acquired)
				release();
		}

		void acquire()
		{
			assert(!m_acquired);

			m_mutex.acquire();

			m_acquired = true;
		}

		bool acquire(const timeval_t& wait)
		{
			assert(!m_acquired);

			if (m_mutex.acquire(&wait))
				m_acquired = true;

			return m_acquired;
		}

		void release()
		{
			assert(m_acquired);

			m_acquired = false;

			m_mutex.release();
		}

	private:
		Guard(const Guard&) {}
		Guard& operator = (const Guard&) { return *this; }

		bool   m_acquired;
		MUTEX& m_mutex;
	};

	template <typename MUTEX>
	class ReadGuard
	{
	public:
		ReadGuard(MUTEX& mutex, bool acq = true) :
		  m_acquired(false),
		  m_mutex(mutex)
		{
			if (acq)
				acquire();
		}

		~ReadGuard()
		{
			if (m_acquired)
				release();
		}

		void acquire()
		{
			assert(!m_acquired);

			m_mutex.acquire_read();

			m_acquired = true;
		}

		void release()
		{
			assert(m_acquired);

			m_acquired = false;

			m_mutex.release_read();
		}

	private:
		ReadGuard(const ReadGuard&) {}
		ReadGuard& operator = (const ReadGuard&) { return *this; }

		bool   m_acquired;
		MUTEX& m_mutex;
	};
}

#endif // OOBASE_MUTEX_H_INCLUDED_
