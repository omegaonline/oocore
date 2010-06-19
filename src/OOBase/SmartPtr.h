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

#ifndef OOBASE_SMARTPTR_H_INCLUDED_
#define OOBASE_SMARTPTR_H_INCLUDED_

#include "Atomic.h"

namespace OOBase
{
	template <typename T>
	class DeleteDestructor
	{
	public:
		static void destroy(T* ptr)
		{
			delete ptr;
		}
	};

	template <typename T>
	class ArrayDestructor
	{
	public:
		static void destroy(T* ptr)
		{
			delete [] ptr;
		}
	};

	template <typename T>
	class FreeDestructor
	{
	public:
		static void destroy(T* ptr)
		{
			::free(ptr);
		}
	};

	namespace detail
	{
		template <typename T, typename Destructor = DeleteDestructor<T> >
		class SmartPtrImpl
		{
			class SmartPtrNode
			{
			public:
				SmartPtrNode(T* data = 0) :
						m_data(data),
						m_refcount(1)
				{}

				void addref()
				{
					++m_refcount;
				}

				void release()
				{
					if (--m_refcount == 0)
						delete this;
				}

				T* value()
				{
					return m_data;
				}

				const T* value() const
				{
					return m_data;
				}

				T* detach()
				{
					T* d = m_data;
					m_data = 0;
					return d;
				}

			private:
				~SmartPtrNode()
				{
					Destructor::destroy(m_data);
				}

				T*                m_data;
				AtomicInt<size_t> m_refcount;
			};

		public:
			SmartPtrImpl(T* ptr = 0) : m_node(0)
			{
				if (ptr)
				{
					OOBASE_NEW(m_node,SmartPtrNode(ptr));
					if (!m_node)
						OOBase_OutOfMemory();
				}
			}

			SmartPtrImpl(const SmartPtrImpl& rhs) : m_node(rhs.m_node)
			{
				if (m_node)
					m_node->addref();
			}

			SmartPtrImpl& operator = (T* ptr)
			{
				if (m_node)
				{
					m_node->release();
					m_node = 0;
				}

				if (ptr)
				{
					OOBASE_NEW(m_node,SmartPtrNode(ptr));
					if (!m_node)
						OOBase_OutOfMemory();
				}

				return *this;
			}

			SmartPtrImpl& operator = (const SmartPtrImpl& rhs)
			{
				if (this != &rhs)
				{
					if (m_node)
					{
						m_node->release();
						m_node = 0;
					}

					m_node = rhs.m_node;

					if (m_node)
						m_node->addref();
				}
				return *this;
			}

			~SmartPtrImpl()
			{
				if (m_node)
					m_node->release();
			}

			T* detach()
			{
				T* v = 0;
				if (m_node)
				{
					v = m_node->detach();
					m_node->release();
					m_node = 0;
				}
				return v;
			}

			operator T*()
			{
				return value();
			}

			operator const T*() const
			{
				return value();
			}

		protected:
			T* value()
			{
				return (m_node ? m_node->value() : 0);
			}

			const T* value() const
			{
				return (m_node ? m_node->value() : 0);
			}

		private:
			SmartPtrNode* m_node;
		};
	}

	template <typename T, typename Destructor = DeleteDestructor<T> >
	class SmartPtr : public detail::SmartPtrImpl<T,Destructor>
	{
		typedef detail::SmartPtrImpl<T,Destructor> baseClass;
	public:
		SmartPtr(T* ptr = 0) : baseClass(ptr)
		{}

		SmartPtr(const SmartPtr& rhs) : baseClass(rhs)
		{}

		SmartPtr& operator = (T* ptr)
		{
			baseClass::operator=(ptr);
			return *this;
		}

		SmartPtr& operator = (const SmartPtr& rhs)
		{
			if (this != &rhs)
				baseClass::operator=(rhs);

			return *this;
		}

		T* operator ->()
		{
			assert(baseClass::value() != 0);

			return baseClass::value();
		}

		const T* operator ->() const
		{
			assert(baseClass::value() != 0);

			return baseClass::value();
		}
	};

	template <typename Destructor>
	class SmartPtr<void,Destructor> : public detail::SmartPtrImpl<void,Destructor>
	{
		typedef detail::SmartPtrImpl<void,Destructor> baseClass;
	public:
		SmartPtr(void* ptr = 0) : baseClass(ptr)
		{}

		SmartPtr(const SmartPtr& rhs) : baseClass(rhs)
		{}

		SmartPtr& operator = (void* ptr)
		{
			baseClass::operator=(ptr);
			return *this;
		}

		SmartPtr& operator = (const SmartPtr& rhs)
		{
			if (this != &rhs)
				baseClass::operator=(rhs);

			return *this;
		}

		template <typename T2>
		operator T2*()
		{
			return static_cast<T2*>(baseClass::value());
		}

		template <typename T2>
		operator const T2*() const
		{
			return static_cast<T2*>(baseClass::value());
		}
	};
}

#endif // OOBASE_SMARTPTR_H_INCLUDED_
