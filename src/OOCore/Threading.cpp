///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
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

#include "OOCore_precomp.h"

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,cs__ctor,0,())
{
	ACE_Recursive_Thread_Mutex* pm = 0;
	OMEGA_NEW(pm,ACE_Recursive_Thread_Mutex);
	return pm;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(cs__dctor,1,((in),void*,m1))
{
	delete static_cast<ACE_Recursive_Thread_Mutex*>(m1);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(cs_lock,1,((in),void*,m1))
{
	if (static_cast<ACE_Recursive_Thread_Mutex*>(m1)->acquire() != 0)
		OOCORE_THROW_LASTERROR();
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(cs_unlock,1,((in),void*,m1))
{
	if (static_cast<ACE_Recursive_Thread_Mutex*>(m1)->release() != 0)
		OOCORE_THROW_LASTERROR();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,rw_lock__ctor,0,())
{
	ACE_RW_Thread_Mutex* pm = 0;
	OMEGA_NEW(pm,ACE_RW_Thread_Mutex);
	return pm;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(rw_lock__dctor,1,((in),void*,m1))
{
	delete static_cast<ACE_RW_Thread_Mutex*>(m1);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(rw_lock_lockread,1,((in),void*,m1))
{
	if (static_cast<ACE_RW_Thread_Mutex*>(m1)->acquire_read() != 0)
		OOCORE_THROW_LASTERROR();
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(rw_lock_lockwrite,1,((in),void*,m1))
{
	if (static_cast<ACE_RW_Thread_Mutex*>(m1)->acquire_write() != 0)
		OOCORE_THROW_LASTERROR();
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(rw_lock_unlock,1,((in),void*,m1))
{
	if (static_cast<ACE_RW_Thread_Mutex*>(m1)->release() != 0)
		OOCORE_THROW_LASTERROR();
}
