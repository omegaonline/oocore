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

#ifndef OOBASE_CONDITION_H_INCLUDED_
#define OOBASE_CONDITION_H_INCLUDED_

#include "Mutex.h"
#include "TimeVal.h"
#include "Win32.h"

namespace OOBase
{
	class Condition
	{
	public:

#if defined(_WIN32)
		typedef Win32::condition_mutex_t Mutex;
#elif defined(HAVE_PTHREAD)
		typedef OOBase::Mutex Mutex;
#else
#error Fix me!
#endif

		Condition();
		~Condition();

		bool wait(Condition::Mutex& mutex, const timeval_t* wait = 0);
		void signal();
		void broadcast();

	private:
		Condition(const Condition&);
		Condition& operator = (const Condition&);

#if defined(_WIN32)
		CONDITION_VARIABLE m_var;
#elif defined(HAVE_PTHREAD)
		pthread_cond_t     m_var;
#else
#error Fix me!
#endif
	};
}

#endif // OOBASE_CONDITION_H_INCLUDED_
