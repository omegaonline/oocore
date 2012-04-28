///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
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

#ifndef OMEGA_MEMORY_H_INCLUDED_
#define OMEGA_MEMORY_H_INCLUDED_

namespace Omega
{
	namespace System
	{
		void* Allocate(size_t bytes);
		void Free(void* mem);

		namespace Internal
		{
			class ThrowingNew
			{
			public:
				// Custom new and delete
				void* operator new(size_t size)
				{
					return Omega::System::Allocate(size);
				}

				void* operator new[](size_t size)
				{
					return Omega::System::Allocate(size);
				}

				void operator delete(void* p)
				{
					Omega::System::Free(p);
				}

				void operator delete[](void* p)
				{
					Omega::System::Free(p);
				}
			};
		}

		template <typename T>
		class STLAllocator
		{
		public:
			typedef size_t    size_type;
			typedef ptrdiff_t difference_type;
			typedef T*        pointer;
			typedef const T*  const_pointer;
			typedef T&        reference;
			typedef const T&  const_reference;
			typedef T         value_type;

			template <typename U>
			struct rebind
			{
				typedef STLAllocator<U> other;
			};

			STLAllocator() throw()
			{ }

			STLAllocator(const STLAllocator&) throw()
			{ }

			template <typename U>
			STLAllocator(const STLAllocator<U>&) throw()
			{ }

			~STLAllocator() throw ()
			{ }

			pointer address(reference x) const
			{
				return &x;
			}

			const_pointer address(const_reference x) const
			{
				return &x;
			}

			pointer allocate(size_type n, const void* = NULL)
			{
				return static_cast<T*>(Omega::System::Allocate(n * sizeof(T)));
			}

			void deallocate(pointer p, size_type)
			{
				Omega::System::Free(p);
			}

			size_type max_size() const throw ()
			{
				return size_t(-1) / sizeof(T);
			}

			void construct(pointer p, const T& val)
			{
				::new (static_cast<void*>(p)) T(val);
			}

			void destroy(pointer p)
			{
				OMEGA_UNUSED_ARG(p);
				p->~T();
			}
		};

		template <typename T1, typename T2>
		inline bool operator == (const STLAllocator<T1>&, const STLAllocator<T2>&)
		{
			return true;
		}

		template <typename T1, typename T2>
		inline bool operator != (const STLAllocator<T1>&, const STLAllocator<T2>&)
		{
			return false;
		}
	}
}

#endif // OMEGA_MEMORY_H_INCLUDED_
