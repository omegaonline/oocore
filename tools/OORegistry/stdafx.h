#ifdef WIN32
#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0500
#endif
#endif

#define ACE_AS_STATIC_LIBS 1
#define ACE_USES_WCHAR

#include <ace/ARGV.h>
#include <ace/Get_Opt.h>
#include <ace/OS.h>

#include <OTL/OTL.h>
