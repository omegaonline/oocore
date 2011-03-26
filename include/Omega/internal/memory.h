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

		template <typename T>
		class stl_allocator
		{
		public:
			// type definitions
			typedef T              value_type;
			typedef T*             pointer;
			typedef const T*       const_pointer;
			typedef T&             reference;
			typedef const T&       const_reference;
			typedef std::size_t    size_type;
			typedef std::ptrdiff_t difference_type;

			// rebind allocator to type U
			template <typename U>
			struct rebind 
			{
				typedef stl_allocator<U> other;
			};

			// return address of values
			pointer address(reference value) const 
			{
				return &value;
			}

			const_pointer address(const_reference value) const
			{
				return &value;
			}

			// constructors and destructor
			// - nothing to do because the allocator has no state
			stl_allocator() throw() 
			{}

			stl_allocator(const stl_allocator&) throw() 
			{}

			template <typename U>
			stl_allocator (const stl_allocator<U>&) throw() 
			{}

			template<typename U>
			stl_allocator& operator = (const stl_allocator<U>&) throw()
			{
				return *this;
			}

			~stl_allocator() throw() 
			{}

			// return maximum number of elements that can be allocated
			size_type max_size() const throw() 
			{
			   return std::numeric_limits<size_type>::max() / sizeof(T);
			}

			// initialize elements of allocated storage p with value value
			void construct(pointer p, const T& value) 
			{
				// initialize memory with placement new
				::new (static_cast<void*>(p)) T(value);
			}

			// destroy elements of initialized storage p
			void destroy(pointer p) 
			{
				// destroy objects by calling their destructor
				if (p) 
					p->~T();
			}

			// allocate but don't initialize num elements of type T
			pointer allocate(size_type num, const void* = 0) 
			{
				return static_cast<pointer>(Omega::System::Allocate(num*sizeof(T)));
			}

			// deallocate storage p of deleted elements
			void deallocate(pointer p, size_type /*num*/) 
			{
				Omega::System::Free(p);
			}
		};

		// return that all specializations of this allocator are interchangeable
		template <typename T1, typename T2>
		bool operator == (const stl_allocator<T1>&, const stl_allocator<T2>&) throw()
		{
			return true;
		}

		template <typename T1, typename T2>
		bool operator != (const stl_allocator<T1>& a1, const stl_allocator<T2>& a2) throw()
		{
			return !(a1 == a2);
		}
	}
}

#endif // OMEGA_MEMORY_H_INCLUDED_
