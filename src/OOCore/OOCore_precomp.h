#ifndef OOCORE_LOCAL_MACROS_H_INCLUDED_
#define OOCORE_LOCAL_MACROS_H_INCLUDED_

#ifndef OOCORE_BUILD_DLL
#define OOCORE_BUILD_DLL
#endif

// Pre include config...
#include <OOCore/config.h>

/////////////////////////////////////////////////
// Include ACE components

#include <ace/OS.h>
#include <ace/Singleton.h>
#include <ace/DLL_Manager.h>
#include <ace/DLL.h>
#include <ace/Thread_Mutex.h>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/Process.h>
#include <ace/MEM_Connector.h>
#include <ace/Auto_Ptr.h>
#include <ace/TSS_T.h>
#include <ace/Task.h>
#include <ace/Activation_Queue.h>
#include <ace/Method_Request.h>
#include <ace/Reactor.h>
#include <ace/Naming_Context.h>
#include <ace/Codecs.h>
#include <ace/SString.h>
#include <ace/CDR_Stream.h>

// New ones...
#include <ace/Proactor.h>

#if defined(ACE_WIN32)
#include <ace/WFMO_Reactor.h>
#else
#include <ace/TP_Reactor.h>
#endif

// End of ACE includes
/////////////////////////////////////////////////

/////////////////////////////////////////////////
// Include STL components

#if (defined(_MSC_VER) && _MSC_VER>=1300)
// Some stl functions have warnings
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

#include <list>
#include <stdexcept>
#include <map>
#include <vector>
#include <set>

#if (defined(_MSC_VER) && _MSC_VER>=1300)
// Some stl functions have warnings
#pragma warning(pop)
#endif

// End of STL includes
/////////////////////////////////////////////////

/////////////////////////////////////////////////
// Include OOCore/OTL components

#include <OOCore/Registry.h>
#include <OOCore/ROT.h>
#include <OOCore/Remoting.h>
#include <OTL/OTL.h>

// End of OOCore/OTL includes
/////////////////////////////////////////////////

/////////////////////////////////////////////////
// Include Platform SDK components

#ifdef OMEGA_WIN32
// For the Windows path functions
#include <shlwapi.h>
#include <shlobj.h>
#endif

// End of Platform SDK includes
/////////////////////////////////////////////////

#define OOCORE_THROW_LASTERROR() \
	OMEGA_THROW(ACE_OS::strerror(ACE_OS::last_error()))

#define OOCORE_THROW_ERRNO(error) \
	OMEGA_THROW(ACE_OS::strerror(error))

#endif // OOCORE_LOCAL_MACROS_H_INCLUDED_

