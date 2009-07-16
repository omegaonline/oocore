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
		class Mutex
		{
		public:
			inline Mutex();
			inline ~Mutex();

			inline void Acquire();
			inline void Release();

		private:
			Mutex(const Mutex&);
			Mutex& operator = (const Mutex&);

			struct handle_t
			{
				int unused;
			}* m_handle;
		};

		class ReaderWriterLock
		{
		public:
			inline ReaderWriterLock();
			inline ~ReaderWriterLock();

			inline void AcquireRead();
			inline void Acquire();
			inline void ReleaseRead();
			inline void Release();

		private:
			ReaderWriterLock(const ReaderWriterLock&);
			ReaderWriterLock& operator = (const ReaderWriterLock&);

			struct handle_t
			{
				int unused;
			}* m_handle;
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
					Acquire();
			}

			~Guard()
			{
				if (m_acquired)
					Release();
			}

			void Acquire()
			{
				m_mutex.Acquire();

				m_acquired = true;
			}

			void Release()
			{
				m_acquired = false;

				m_mutex.Release();
			}

		private:
			Guard(const Guard&);
			Guard& operator = (const Guard&);

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
					Acquire();
			}

			~ReadGuard()
			{
				if (m_acquired)
					Release();
			}

			void Acquire()
			{
				m_mutex.AcquireRead();

				m_acquired = true;
			}

			void Release()
			{
				m_acquired = false;

				m_mutex.ReleaseRead();
			}

		private:
			ReadGuard(const ReadGuard&);
			ReadGuard& operator = (const ReadGuard&);

			bool   m_acquired;
			MUTEX& m_mutex;
		};

		class AtomicRefCount
		{
		public:
			inline AtomicRefCount();
			inline ~AtomicRefCount();

			inline bool AddRef();
			inline bool Release();
			inline bool IsZero() const;

#ifdef OMEGA_DEBUG
			size_t m_debug_value;
#endif

		private:
			struct handle_t
			{
				int unused;
			}* m_handle;
		};

		template <typename DLL>
		class ModuleDestructor
		{
		public:
			typedef void (OMEGA_CALL *pfn_destructor)(void*);

			inline static void add_destructor(pfn_destructor pfn, void* param);
			inline static void remove_destructor(pfn_destructor pfn, void* param);
			
		private:
			ModuleDestructor(const ModuleDestructor&);
			ModuleDestructor& operator = (const ModuleDestructor&);

			ModuleDestructor() 
			{
			}

			inline ~ModuleDestructor();

			Mutex                                       m_lock;
			std::list<std::pair<pfn_destructor,void*> > m_list;

			static ModuleDestructor& instance()
			{
				static ModuleDestructor inst;
				return inst;
			}
		};

		template <typename DLL>
		class InitialiseDestructor
		{
		public:
			typedef void (OMEGA_CALL* pfn_destructor)(void*);

			inline static void add_destructor(pfn_destructor pfn, void* param);

		private:
			struct multi_dctor
			{
				pfn_destructor pfn;
				void*          param;
			};

			inline static void OMEGA_CALL destruct(void*);
		};

		// Lifetime should be either ModuleDestructor<> or InitialiseDestructor
		template <typename T, typename Lifetime>
		class Singleton
		{
		public:
			static T* instance();

		private:
			static void* s_instance;

			static void OMEGA_CALL do_init()
			{
				s_instance = new T();
				Lifetime::add_destructor(do_term,0);
			}

			static void OMEGA_CALL do_term(void*)
			{
				delete static_cast<T*>(s_instance);
				s_instance = 0;
			}
		};
	}
}

#endif // OMEGA_THREADING_H_INCLUDED_
