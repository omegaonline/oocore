#include <OOCore/OOCore.h>

#if (defined(_MSC_VER) && _MSC_VER>=1300)
#pragma warning(disable : 4541)
#endif

#include <ace/Connector.h>

#if (defined(_MSC_VER) && _MSC_VER>=1300)
#pragma warning(default : 4541)
#endif

#include <ace/SOCK_Stream.h>
#include <ace/SOCK_Acceptor.h>
#include <ace/SOCK_Connector.h>
#include <ace/Proactor.h>
#include <ace/CDR_Stream.h>

#include <OOCore/Remoting.h>
#include <OTL/OTL.h>

#define OOSERVER_THROW_LASTERROR() \
	OMEGA_THROW(ACE_OS::strerror(ACE_OS::last_error()))

#define OOSERVER_THROW_ERRNO(error) \
	OMEGA_THROW(ACE_OS::strerror(error))

#if defined(ACE_NLOGGING)
#error You must not define ACE_NLOGGING, cos we use it!
#endif
