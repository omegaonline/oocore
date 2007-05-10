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
