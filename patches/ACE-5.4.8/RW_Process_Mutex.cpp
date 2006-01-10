// RW_Process_Mutex.cpp,v 4.11 2004/06/14 13:58:41 jwillemsen Exp

// Changed by rickt on 2005/12/12

#include "ace/RW_Process_Mutex.h"
#include "ace/Log_Msg.h"
#include "ace/ACE.h"

ACE_RCSID(ace, RW_Process_Mutex, "RW_Process_Mutex.cpp,v 4.12 2005/10/28 16:14:55 ossama Exp")

#if !defined (__ACE_INLINE__)
#include "ace/RW_Process_Mutex.inl"
#endif /* __ACE_INLINE__ */

#include "ace/Malloc_T.h"

#if defined (ACE_WIN32)
#include "ace/OS_NS_fcntl.h"
#endif /* ACE_WIN32 */

#include "ace/OS_NS_sys_stat.h"
#include "ace/OS_NS_unistd.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_ALLOC_HOOK_DEFINE(ACE_RW_Process_Mutex)

const ACE_TCHAR *
ACE_RW_Process_Mutex::unique_name (void)
{
  ACE::unique_name (this, this->name_, ACE_UNIQUE_NAME_LEN);
  return this->name_;
}

ACE_RW_Process_Mutex::ACE_RW_Process_Mutex (const ACE_TCHAR *name,
                                            int flags,
	                                        mode_t mode )
  : lock_ (name ? name : this->unique_name (), flags, mode, 0)
{
	lock_.acquire();

	long ref_count = 0;
	ACE_OS::pread(lock_.get_handle(),&ref_count,sizeof(ref_count),0);
	
	++ref_count;
	ACE_OS::pwrite(lock_.get_handle(),&ref_count,sizeof(ref_count),0);

	lock_.release();

// ACE_TRACE ("ACE_RW_Process_Mutex::ACE_RW_Process_Mutex");
}

ACE_RW_Process_Mutex::~ACE_RW_Process_Mutex (void)
{
	lock_.acquire();

	long ref_count;
	if (ACE_OS::pread(lock_.get_handle(),&ref_count,sizeof(ref_count),0) == sizeof(ref_count))
	{
		if (--ref_count==0)
			lock_.remove();
		else
			ACE_OS::pwrite(lock_.get_handle(),&ref_count,sizeof(ref_count),0);
	}

	lock_.release();

// ACE_TRACE ("ACE_RW_Process_Mutex::~ACE_RW_Process_Mutex");
}

void
ACE_RW_Process_Mutex::dump (void) const
{
#if defined (ACE_HAS_DUMP)
// ACE_TRACE ("ACE_RW_Process_Mutex::dump");
  ACE_DEBUG ((LM_DEBUG, ACE_BEGIN_DUMP, this));
  this->lock_.dump ();
  ACE_DEBUG ((LM_DEBUG, ACE_END_DUMP));
#endif /* ACE_HAS_DUMP */
}

//
// These are instantiated both with and without ACE_HAS_THREADS.
//
#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)

// template class ACE_Guard<ACE_RW_Process_Mutex>;
template class ACE_Malloc_Lock_Adapter_T<ACE_RW_Process_Mutex>;

#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)

// #pragma instantiate ACE_Guard<ACE_RW_Process_Mutex>
#pragma instantiate ACE_Malloc_Lock_Adapter_T<ACE_RW_Process_Mutex>

#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */

ACE_END_VERSIONED_NAMESPACE_DECL
