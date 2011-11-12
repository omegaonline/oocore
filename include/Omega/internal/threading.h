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
	// Forward declare
	namespace System
	{
		namespace Internal
		{
			struct SafeShim;
		}
	}

	namespace Threading
	{
		class Mutex
		{
		public:
			Mutex();
			~Mutex();

			void Acquire();
			void Release();

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
			ReaderWriterLock();
			~ReaderWriterLock();

			void AcquireRead();
			void Acquire();
			void ReleaseRead();
			void Release();

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
			AtomicRefCount() : m_value(0)
			{}

			~AtomicRefCount()
			{}

			size_t AddRef();
			size_t Release();
			bool IsZero() const;
			
		private:
			size_t m_value;
		};

		typedef void (OMEGA_CALL *DestructorCallback)(void*);

		template <typename DLL>
		class ModuleDestructor
		{
		public:
			static void add_destructor(DestructorCallback pfn, void* param);
			static void remove_destructor(DestructorCallback pfn, void* param);

		private:
			ModuleDestructor();
			ModuleDestructor(const ModuleDestructor&);
			ModuleDestructor& operator = (const ModuleDestructor&);
			
			~ModuleDestructor();

			struct auto_destructor
			{
				auto_destructor(void*& h) : m_h(h)
				{}

				~auto_destructor()
				{
					OOCore_mod_destruct__dctor(m_h);
				}

				void*& m_h;

			private:
				auto_destructor(const auto_destructor& rhs) : m_h(rhs.m_h) {}
				auto_destructor& operator = (const auto_destructor&) { return *this; }
			};

			static void* handle();
		};

		template <typename DLL>
		class InitialiseDestructor
		{
		public:
			static void add_destructor(DestructorCallback pfn, void* param);

		private:
			struct multi_dctor
			{
				multi_dctor(DestructorCallback pfn, void* p) :
					pfn_dctor(pfn), param(p)
				{}

				DestructorCallback pfn_dctor;
				void*              param;
			};

			static void OMEGA_CALL destruct(void* param);
		};

		typedef const System::Internal::SafeShim* (OMEGA_CALL *SingletonCallback)(void** param);
		
		// Lifetime should be either ModuleDestructor<> or InitialiseDestructor
		template <typename T, typename Lifetime>
		class Singleton
		{
		public:
			static T* instance();

		private:
			static const System::Internal::SafeShim* OMEGA_CALL do_init(void** param);
			static void OMEGA_CALL do_term(void*);
		};
	}

	namespace System
	{
		namespace Internal
		{
			// Callback is C ABI safe
			template <>
			struct is_c_abi<const SafeShim*()>
			{
				enum { result = 1 };
			};
		}
	}
}

#endif // OMEGA_THREADING_H_INCLUDED_
