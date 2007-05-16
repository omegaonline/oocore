#ifndef OOCORE_LOCAL_MACROS_H_INCLUDED_
#define OOCORE_LOCAL_MACROS_H_INCLUDED_

#ifndef OOCORE_BUILD_LIBRARY
#define OOCORE_BUILD_LIBRARY
#endif

// Pre include config...
#include <OOCore/config.h>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#endif

/////////////////////////////////////////////////
// Include ACE components

// Link to the static lib version of ACE...
#define ACE_AS_STATIC_LIBS 1

#include <ace/OS.h>
#include <ace/Singleton.h>
#include <ace/DLL_Manager.h>
#include <ace/DLL.h>
#include <ace/Thread_Mutex.h>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/RW_Thread_Mutex.h>
#include <ace/Process.h>
#include <ace/MEM_Connector.h>
#include <ace/Auto_Ptr.h>
#include <ace/TSS_T.h>
#include <ace/Task.h>
#include <ace/Activation_Queue.h>
#include <ace/Method_Request.h>
#include <ace/Reactor.h>
#include <ace/Naming_Context.h>
#include <ace/NT_Service.h>
#include <ace/Codecs.h>
#include <ace/SString.h>
#include <ace/CDR_Stream.h>
#include <ace/Proactor.h>
#include <ace/UUID.h>

#if defined(OMEGA_WIN32)
#include <ace/WFMO_Reactor.h>
#else
#include <ace/TP_Reactor.h>
#endif

// End of ACE includes
/////////////////////////////////////////////////

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

/////////////////////////////////////////////////
// Include OOCore/OTL components

#include <OOCore/Remoting.h>
#include <OTL/OTL.h>

// End of OOCore/OTL includes
/////////////////////////////////////////////////

#define OOCORE_THROW_LASTERROR() \
	OMEGA_THROW(ACE_OS::strerror(ACE_OS::last_error()))

#define OOCORE_THROW_ERRNO(error) \
	OMEGA_THROW(ACE_OS::strerror(error))

#define OOCORE_GUARD(MUTEX, OBJ, LOCK) \
	ACE_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OOCORE_THROW_LASTERROR();

#define OOCORE_READ_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Read_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OOCORE_THROW_LASTERROR();

#define OOCORE_WRITE_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Write_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OOCORE_THROW_LASTERROR();


#endif // OOCORE_LOCAL_MACROS_H_INCLUDED_

