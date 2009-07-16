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

#include "Once.h"

#if defined(_WIN32)

namespace
{
	static BOOL __stdcall PINIT_ONCE_FN_impl(INIT_ONCE* /*InitOnce*/, void* Parameter, void** /*Context*/)
	{
		(*((OOBase::Once::pfn_once)Parameter))();
		return TRUE;
	}
}

void OOBase::Once::Run(once_t* key, pfn_once fn)
{
	void* p = 0;
	if (!Win32::InitOnceExecuteOnce(key,&PINIT_ONCE_FN_impl,(void*)fn,&p))
		OOBase_CallCriticalFailure(GetLastError());
}

#elif defined(HAVE_PTHREAD)

void OOBase::Once::Run(once_t* key, pfn_once fn)
{
	int err = pthread_once(key,fn);
	if (err != 0)
		OOBase_CallCriticalFailure(err);
}

#else

#error Fix me!

#endif
