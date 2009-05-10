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

#ifndef OOBASE_SINGLETON_H_INCLUDED_
#define OOBASE_SINGLETON_H_INCLUDED_

#include "Mutex.h"
#include "Destructor.h"
#include "Once.h"

namespace OOBase
{
	template <typename T, typename DLL = int>
	class SingletonNoDestroy
	{
	public:
		static T* instance()
		{
			static Once::once_t key = ONCE_T_INIT;
			Once::Run(&key,init);
			return s_instance;
		}

		static void close()
		{
			delete s_instance;
			//s_instance = reinterpret_cast<T*>((uintptr_t)0xdeadbeef);
		}

	protected:
		static T* s_instance;

	private:
		// Prevent creation
		SingletonNoDestroy() {}
		SingletonNoDestroy(const SingletonNoDestroy&) {}
		SingletonNoDestroy& operator == (const SingletonNoDestroy&) { return *this; }
		~SingletonNoDestroy() {}

		static void init()
		{
			OOBASE_NEW(s_instance,T());
			if (!s_instance)
				OOBase_OutOfMemory();
		}
	};

	template <typename T, typename DLL = int>
	class Singleton : public SingletonNoDestroy<T,DLL>
	{
	public:
		static T* instance()
		{
			static Once::once_t key = ONCE_T_INIT;
			Once::Run(&key,init);
			return SingletonNoDestroy<T,DLL>::s_instance;
		}

		static void close()
		{
			DLLDestructor<DLL>::remove_destructor(&destroy,0);
			destroy();
		}

	private:
		// Prevent creation
		Singleton() {}
		Singleton(const Singleton&) {}
		Singleton& operator == (const Singleton&) { return *this; }
		~Singleton() {}

		static void init()
		{
			T* t;
			OOBASE_NEW(t,T());
			if (!t)
				OOBase_OutOfMemory();

			SingletonNoDestroy<T,DLL>::s_instance = t;

			DLLDestructor<DLL>::add_destructor(&destroy,0);
		}

		static void destroy(void* = 0)
		{
			delete SingletonNoDestroy<T,DLL>::s_instance;
			//SingletonNoDestroy<T,DLL>::s_instance = reinterpret_cast<T*>((uintptr_t)0xdeadbeef);
		}
	};

	template <typename T, typename DLL>
	T* SingletonNoDestroy<T,DLL>::s_instance = 0;
}

#endif // OOBASE_SINGLETON_H_INCLUDED_
