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

#ifndef OMEGA_THREADING_H_INCLUDED_
#define OMEGA_THREADING_H_INCLUDED_

namespace Omega
{
	namespace Threading
	{
		class CriticalSection
		{
		public:
			inline CriticalSection();
			inline ~CriticalSection();

			inline void Lock();
			inline void Unlock();

		private:
			struct handle_t
			{
				int unused;
			}* m_handle;
		};

		class Guard
		{
		public:
			Guard(CriticalSection& lock) : m_cs(lock)
			{
				m_cs.Lock();
			}

			~Guard()
			{
				m_cs.Unlock();
			}

		private:
			Guard& operator = (const Guard&) { return *this; }

			CriticalSection& m_cs;
		};

		class ReaderWriterLock
		{
		public:
			inline ReaderWriterLock();
			inline ~ReaderWriterLock();

			inline void LockRead();
			inline void LockWrite();
			inline void Unlock();

		private:
			struct handle_t
			{
				int unused;
			}* m_handle;
		};

		class ReadGuard
		{
		public:
			ReadGuard(ReaderWriterLock& lock) : m_lock(lock), m_bLocked(false)
			{
				Lock();
			}

			~ReadGuard()
			{
				if (m_bLocked)
					Unlock();
			}

			void Lock()
			{
				m_lock.LockWrite();
				m_bLocked = true;
			}

			void Unlock()
			{
				try
				{
					m_bLocked = false;
					m_lock.Unlock();
				}
				catch (...)
				{
					// Still locked
					m_bLocked = true;
					throw;
				}
			}

		private:
			ReadGuard& operator = (const ReadGuard&) { return *this; }

			ReaderWriterLock& m_lock;
			bool              m_bLocked;
		};

		class WriteGuard
		{
		public:
			WriteGuard(ReaderWriterLock& lock) : m_lock(lock), m_bLocked(false)
			{
				Lock();
			}

			~WriteGuard()
			{
				if (m_bLocked)
					Unlock();
			}

			void Lock()
			{
				m_lock.LockWrite();
				m_bLocked = true;
			}

			void Unlock()
			{
				try
				{
					m_bLocked = false;
					m_lock.Unlock();
				}
				catch (...)
				{
					// Still locked
					m_bLocked = true;
					throw;
				}
			}

		private:
			WriteGuard& operator = (const WriteGuard&) { return *this; }

			ReaderWriterLock& m_lock;
			bool              m_bLocked;
		};

		template <class T> class AtomicOp
		{
		public:
			AtomicOp() {};
			inline AtomicOp(const T& v);
			inline AtomicOp(const AtomicOp& rhs);

			inline AtomicOp& operator = (const AtomicOp& rhs);
			inline AtomicOp& operator = (const T& rhs);

			inline bool operator == (const AtomicOp& rhs);
			inline bool operator == (const T& rhs);

			inline T operator ++();
			inline T operator ++(int);
			inline T operator --();
			inline T operator --(int);
			inline volatile T* operator &();

			inline T value() const;
			inline T exchange(const T& v);

		private:
			mutable CriticalSection m_cs;
			T                       m_value;
		};

#ifdef OMEGA_HAS_ATOMIC_OP_32

		template <> class AtomicOp<int32_t>
		{
		public:
			AtomicOp() {}
			inline AtomicOp(int32_t v);
			inline AtomicOp(const AtomicOp& rhs);

			inline AtomicOp& operator = (const AtomicOp& rhs);
			inline AtomicOp& operator = (int32_t rhs);

			bool operator == (const AtomicOp& rhs)
			{
				return m_value == rhs.m_value;
			}
			bool operator == (int32_t rhs)
			{
				return m_value == rhs;
			}

			inline int32_t operator ++();
			inline int32_t operator ++(int) { return ++*this - 1; }
			inline int32_t operator --();
			inline int32_t operator --(int) { return --*this + 1; }
			inline volatile int32_t* operator &() { return &m_value; }

			inline int32_t value() const { return m_value; }
			inline int32_t exchange(int32_t v);

		private:
			int32_t m_value;
		};

		template <> class AtomicOp<uint32_t>
		{
		public:
			AtomicOp() {};
			inline AtomicOp(uint32_t v);
			inline AtomicOp(const AtomicOp& rhs);

			inline AtomicOp& operator = (const AtomicOp& rhs);
			inline AtomicOp& operator = (uint32_t rhs);

			bool operator == (const AtomicOp& rhs)
			{
				return m_value == rhs.m_value;
			}
			bool operator == (uint32_t rhs)
			{
				return m_value == rhs;
			}

			inline uint32_t operator ++();
			inline uint32_t operator ++(int) { return ++*this - 1; }
			inline uint32_t operator --();
			inline uint32_t operator --(int) { return --*this + 1; }
			inline volatile uint32_t* operator &()  { return &m_value; }

			inline uint32_t value() const  { return m_value; }
			inline uint32_t exchange(uint32_t v);

		private:
			uint32_t	m_value;
		};

#if !defined(OMEGA_64)
		template <class T> class AtomicOp<T*>
		{
		public:
			AtomicOp() {};
			inline AtomicOp(T* v);
			inline AtomicOp(const AtomicOp& rhs);

			inline AtomicOp& operator = (const AtomicOp& rhs);
			inline AtomicOp& operator = (T* rhs);

			bool operator == (const AtomicOp& rhs)
			{
				return m_value == rhs.m_value;
			}
			bool operator == (T* rhs)
			{
				return m_value == rhs;
			}

			inline T* value() const  { return m_value; }
			inline T* exchange(T* v);

		private:
			T*	m_value;
		};
#endif

#endif // OMEGA_HAS_ATOMIC_OP_32
	}
}

#endif // OMEGA_THREADING_H_INCLUDED_
