#include <OOCore/OOCore.h>

#if defined(ACE_NLOGGING)
#error You must not define ACE_NLOGGING, cos we use it!
#endif

#define ACE_AS_STATIC_LIBS 1

#include <ace/Asynch_Acceptor.h>
#include <ace/CDR_Stream.h>
#include <ace/Configuration.h>
#include <ace/Connector.h>
#include <ace/OS.h>
#include <ace/Proactor.h>
#include <ace/SOCK_Connector.h>

#include <OOCore/Remoting.h>
#include <OTL/OTL.h>

#include <set>

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
