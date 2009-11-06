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

#ifndef OOBASE_ONCE_H_INCLUDED_
#define OOBASE_ONCE_H_INCLUDED_

#include "Win32.h"

namespace OOBase
{
	namespace Once
	{
		/** \typedef once_t
		 *  The platform specific 'once' type.
		 */

		/** \def ONCE_T_INIT 
		 *  The platform specific 'once' type initialiser
		 */

#if defined(_WIN32)
		typedef INIT_ONCE once_t;
		#define ONCE_T_INIT {0}
#elif defined(HAVE_PTHREAD)
		typedef pthread_once_t once_t;
		#define ONCE_T_INIT PTHREAD_ONCE_INIT
#else
#error Fix Me!
#endif

		typedef void (*pfn_once)(void);

		void Run(once_t* key, pfn_once fn);
	}
}

#endif // OOBASE_ONCE_H_INCLUDED_
