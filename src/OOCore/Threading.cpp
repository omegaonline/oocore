///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
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

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_sngtn_once,3,((in),void**,val,(in),void*,pfn_init,(in),void*,pfn_term))
{
	typedef bool (*init_function)();
	typedef void (*term_function)(void*);

	// Do a double lock...
	if (!*val)
	{
		// This singleton should be race start safe...
		OOBase::Guard<OOBase::SpinLock> guard(*OOBase::Singleton<OOBase::SpinLock>::instance());

		if (!*val)
		{
			// Call the init function
			if (!(*(init_function)(pfn_init))())
			{
				OOBase::Destructor::add_destructor((term_function)(pfn_term),0);
			}
		}
	}
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_cs__ctor,0,())
{
	OOBase::Mutex* pm = 0;
	OMEGA_NEW(pm,OOBase::Mutex);
	return pm;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_cs__dctor,1,((in),void*,m1))
{
	delete static_cast<OOBase::Mutex*>(m1);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_cs_lock,1,((in),void*,m1))
{
	static_cast<OOBase::Mutex*>(m1)->acquire();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_cs_unlock,1,((in),void*,m1))
{
	static_cast<OOBase::Mutex*>(m1)->release();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_rw_lock__ctor,0,())
{
	OOBase::RWMutex* pm = 0;
	OMEGA_NEW(pm,OOBase::RWMutex);
	return pm;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock__dctor,1,((in),void*,m1))
{
	delete static_cast<OOBase::RWMutex*>(m1);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_lockread,1,((in),void*,m1))
{
	static_cast<OOBase::RWMutex*>(m1)->acquire_read();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_lockwrite,1,((in),void*,m1))
{
	static_cast<OOBase::RWMutex*>(m1)->acquire();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_unlockread,1,((in),void*,m1))
{
	static_cast<OOBase::RWMutex*>(m1)->release_read();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_rw_lock_unlockwrite,1,((in),void*,m1))
{
	static_cast<OOBase::RWMutex*>(m1)->release();
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(void*,OOCore_atomic__ctor,0,())
{
	OOBase::AtomicInt<size_t>* pm = 0;
	OMEGA_NEW(pm,OOBase::AtomicInt<size_t>);
	return pm;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_atomic__dctor,1,((in),void*,m1))
{
	delete static_cast<OOBase::AtomicInt<size_t>*>(m1);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(OOCore_atomic_addref,1,((in),void*,m1))
{
	++ *static_cast<OOBase::AtomicInt<size_t>*>(m1);
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_atomic_release,1,((in),void*,m1))
{
	return (-- *static_cast<OOBase::AtomicInt<size_t>*>(m1) == 0) ? 1 : 0;
}

OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(int,OOCore_atomic_iszero,1,((in),void*,m1))
{
	return (*static_cast<OOBase::AtomicInt<size_t>*>(m1) == 0) ? 1 : 0;
}
