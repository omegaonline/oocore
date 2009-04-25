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

#ifndef OOBASE_THREAD_H_INCLUDED_
#define OOBASE_THREAD_H_INCLUDED_

#include "TimeVal.h"
#include "Win32.h"

namespace OOBase
{
	class Thread
	{
	public:
		Thread();
		~Thread();

		void run(int (*thread_fn)(void*), void* param);
		bool join(const timeval_t* wait = 0);
		void abort();
		bool is_running();

	private:
		Thread(const Thread&) {}
		Thread& operator = (const Thread&) { return *this; }

#if defined(_WIN32)
		struct wrapper
		{
			Win32::SmartHandle hEvent;
			int                (*thread_fn)(void*);
			void*              param;
		};
		Win32::SmartHandle m_hThread;
		static unsigned int __stdcall dummy_fn(void* param);
#else
#error Fix me!
#endif
	};
}

#endif // OOBASE_THREAD_H_INCLUDED_
