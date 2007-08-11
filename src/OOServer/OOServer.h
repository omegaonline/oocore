
#include <OOCore/OOCore.h>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)

#ifndef _DEBUG
// Optimization sometimes re-orders things causing this error
#pragma warning(disable : 4702)
#endif

#endif

/////////////////////////////////////////////////
// Include ACE components

// Link to the static lib version of ACE...
#define ACE_AS_STATIC_LIBS 1
#define ACE_USES_WCHAR

#include <ace/Asynch_Acceptor.h>
#include <ace/CDR_Stream.h>
#include <ace/Configuration.h>
#include <ace/Connector.h>
#include <ace/Countdown_Time.h>
#include <ace/Event.h>
#include <ace/Message_Queue.h>
#include <ace/OS.h>
#include <ace/Proactor.h>
#include <ace/SOCK_Connector.h>

#include <ace/SPIPE_Acceptor.h>
#include <ace/SPIPE_Connector.h>

// End of ACE includes
/////////////////////////////////////////////////

//////////////////////////////////////////////
// Include STL components

#include <set>

// End of STL includes
//////////////////////////////////////////////

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

/////////////////////////////////////////////////
// Include OOCore/OTL components

#include <OOCore/Remoting.h>
#include <OTL/OTL.h>

// End of OOCore/OTL includes
/////////////////////////////////////////////////

/////////////////////////////////////////////////
// Include Windows components
#if defined(ACE_WIN32)

// We use the unicode CRT
#define _UNICODE

#include <shlobj.h>
#include <shlwapi.h>

#endif
// End of Windows includes
/////////////////////////////////////////////////

#define OOSERVER_THROW_LASTERROR() \
	OMEGA_THROW(ACE_OS::strerror(ACE_OS::last_error()))

#define OOSERVER_THROW_ERRNO(error) \
	OMEGA_THROW(ACE_OS::strerror(error))

#define OOSERVER_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OOSERVER_THROW_LASTERROR();

#define OOSERVER_READ_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Read_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OOSERVER_THROW_LASTERROR();

#define OOSERVER_WRITE_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Write_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OOSERVER_THROW_LASTERROR();
