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

#ifndef OOBASE_DESTRUCTOR_H_INCLUDED_
#define OOBASE_DESTRUCTOR_H_INCLUDED_

#include "Mutex.h"
#include "Once.h"

#include <list>

namespace OOBase
{
	template <typename DLL>
	class DLLDestructor
	{
	public:
		typedef void (*pfn_destructor)(void*);

		static void add_destructor(pfn_destructor pfn, void* p)
		{
			try
			{
				DLLDestructor& inst = instance();
				Guard<SpinLock> guard(inst.m_lock);
				inst.m_list.push_front(std::pair<pfn_destructor,void*>(pfn,p));
			}
			catch (std::exception& e)
			{
				OOBase_CallCriticalFailure(e.what());
			}
		}

		static void remove_destructor(pfn_destructor pfn, void* p)
		{
			try
			{
				DLLDestructor& inst = instance();
				Guard<SpinLock> guard(inst.m_lock);
				inst.m_list.remove(std::pair<pfn_destructor,void*>(pfn,p));
			}
			catch (std::exception& e)
			{
				OOBase_CallCriticalFailure(e.what());
			}
		}

	private:
		DLLDestructor() {}
		DLLDestructor(const DLLDestructor&);
		DLLDestructor& operator = (const DLLDestructor&);

		~DLLDestructor()
		{
			destruct();
		}

		void destruct()
		{
			try
			{
				Guard<SpinLock> guard(m_lock);
			
				for (std::list<std::pair<pfn_destructor,void*> >::iterator i=m_list.begin();i!=m_list.end();++i)
				{
					(*(i->first))(i->second);
				}
				m_list.clear();
			}
			catch (std::exception& e)
			{
				OOBase_CallCriticalFailure(e.what());
			}
		}

		SpinLock m_lock;
		std::list<std::pair<pfn_destructor,void*> > m_list;

		static DLLDestructor& instance()
		{
			static Once::once_t key = ONCE_T_INIT;
			Once::Run(&key,init);

			return *s_instance;
		}

		static void init()
		{
			static DLLDestructor inst;
			s_instance = &inst;
		}

		static DLLDestructor* s_instance;
	};

	template <typename DLL>
	DLLDestructor<DLL>* DLLDestructor<DLL>::s_instance = 0;
}

#endif // OOBASE_DESTRUCTOR_H_INCLUDED_
