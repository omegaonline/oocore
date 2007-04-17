#ifndef OOCORE_CONFIG_MSVC_H_INCLUDED_
#define OOCORE_CONFIG_MSVC_H_INCLUDED_

/*#if (_MSC_VER >= 1400)
# include "ace/config-win32-msvc-8.h"
#elif (_MSC_VER >= 1310)
# include "ace/config-win32-msvc-7.h"
#else
# error This version of Microsoft Visual C++ not supported.
#endif*/

# if !defined(_MT)
// *** DO NOT *** defeat this error message by defining _MT yourself.
// On MSVC, this is changed by selecting the Multithreaded
// DLL or Debug Multithreaded DLL in the Project Settings
// under C++ Code Generation.
#  error You must link against multi-threaded libraries when using OOCore (check your project settings)
# endif // !_MT

#define OMEGA_FUNCNAME		__FUNCSIG__

#if defined (_WIN64)
typedef SSIZE_T ssize_t;
#else
typedef int ssize_t;
#endif /* _WIN64 */

#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
	do { POINTER = new CONSTRUCTOR; \
		if (POINTER == 0) { OMEGA_THROW("Out of memory."); } \
	} while (0)

#define OMEGA_IMPORT __declspec(dllimport)
#define OMEGA_EXPORT __declspec(dllexport)

#if defined(__cplusplus) && !defined(CINTERFACE)
// Windows.h has set some options
#undef DECLARE_INTERFACE
#define DECLARE_INTERFACE(iface)    struct DECLSPEC_NOVTABLE iface
#undef DECLARE_INTERFACE_
#define DECLARE_INTERFACE_(iface, baseiface)    struct DECLSPEC_NOVTABLE iface : public baseiface
#endif

#undef interface
#define interface struct __declspec(novtable)

// Warning 4127 is rubbish!
#pragma warning(disable : 4127)

#endif // OOCORE_CONFIG_MSVC_H_INCLUDED_
