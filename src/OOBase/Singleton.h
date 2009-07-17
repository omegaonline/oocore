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
	template <typename T, typename DLL>
	class Singleton
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
			DLLDestructor<DLL>::remove_destructor(&destroy,0);
			destroy();
		}

	private:
		// Prevent creation
		Singleton();
		Singleton(const Singleton&);
		Singleton& operator = (const Singleton&);
		~Singleton();

		static T* s_instance;

		static void init()
		{
			OOBASE_NEW(s_instance,T());
			if (!s_instance)
				OOBase_OutOfMemory();

			DLLDestructor<DLL>::add_destructor(&destroy,0);
		}

		static void destroy(void* = 0)
		{
			delete s_instance;
			s_instance = reinterpret_cast<T*>((uintptr_t)0xdeadbeef);
		}
	};

	template <typename T, typename DLL>
	T* Singleton<T,DLL>::s_instance = 0;
}

#endif // OOBASE_SINGLETON_H_INCLUDED_
