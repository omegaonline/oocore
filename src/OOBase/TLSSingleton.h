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

#ifndef OOBASE_TLS_SINGLETON_H_INCLUDED_
#define OOBASE_TLS_SINGLETON_H_INCLUDED_

#include "config-base.h"

namespace OOBase
{
	namespace TLS
	{
		void Add(const void* key, void (*destructor)(void*));
		bool Get(const void* key, void** val);
		void Set(const void* key, void* val);
		void Remove(const void* key);

		void ThreadExit();
	}

	template <typename T, typename DLL = int>
	class TLSSingleton
	{
	public:
		static T* instance()
		{
			void* inst = 0;
			if (!TLS::Get(&s_sentinal,&inst))
				inst = init();

			return static_cast<T*>(inst);
		}

		static void close()
		{
			T* i = instance();
			TLS::Remove(&s_sentinal);
			destroy(i);
		}

	private:
		// Prevent creation
		TLSSingleton() {}
		TLSSingleton(const TLSSingleton&) {}
		TLSSingleton& operator == (const TLSSingleton&) { return *this; }
		~TLSSingleton() {}

		static const int s_sentinal = 0;

		static void* init()
		{
			T* pThis = 0;
			OOBASE_NEW(pThis,T());
			if (!pThis)
				OOBase_OutOfMemory();

			TLS::Add(&s_sentinal,&destroy);
			TLS::Set(&s_sentinal,pThis);

			return pThis;
		}

		static void destroy(void* p)
		{
			delete reinterpret_cast<T*>(p);
		}
	};

	template <typename T, typename DLL>
	const int TLSSingleton<T,DLL>::s_sentinal;

}

#endif // OOBASE_TLS_SINGLETON_H_INCLUDED_
