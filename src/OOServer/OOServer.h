#define ACE_HAS_VERSIONED_NAMESPACE  1
#define ACE_AS_STATIC_LIBS

#include <ace/Acceptor.h>

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
#include <ace/Reactor.h>

#include <OOCore/Remoting.h>
#include <OTL/OTL.h>
