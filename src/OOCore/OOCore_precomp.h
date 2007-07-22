#ifndef OOCORE_LOCAL_MACROS_H_INCLUDED_
#define OOCORE_LOCAL_MACROS_H_INCLUDED_

#ifndef OOCORE_BUILD_LIBRARY
#define OOCORE_BUILD_LIBRARY
#endif

// Pre-include config...
#include <OOCore/config-guess.h>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#endif

/////////////////////////////////////////////////
// Include ACE components

// Link to the static lib version of ACE...
#define ACE_AS_STATIC_LIBS 1
#define ACE_USES_WCHAR

#include <ace/CDR_Stream.h>
#include <ace/Codecs.h>
#include <ace/Countdown_Time.h>
#include <ace/DLL.h>
#include <ace/DLL_Manager.h>
#include <ace/Encoding_Converter.h>
#include <ace/Encoding_Converter_Factory.h>
#include <ace/SOCK_Connector.h>
#include <ace/Message_Queue.h>
#include <ace/NT_Service.h>
#include <ace/OS.h>
#include <ace/Proactor.h>
#include <ace/Process.h>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/RW_Thread_Mutex.h>
#include <ace/Singleton.h>
#include <ace/SString.h>
#include <ace/TSS_T.h>
#include <ace/UUID.h>

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

