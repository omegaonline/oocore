#ifndef OOCORE_CONFIG_GCC_H_INCLUDED_
#define OOCORE_CONFIG_GCC_H_INCLUDED_

#define OMEGA_MAX_DEFINES	256

#define OMEGA_FUNCNAME		__PRETTY_FUNCTION__

#include <sys/types.h>
#include <new>

#undef interface
#define interface struct __attribute__((com_interface))

#define OMEGA_NEW(POINTER,CONSTRUCTOR) \
	do { POINTER = new CONSTRUCTOR; \
		if (POINTER == 0) { OMEGA_THROW("Out of memory."); } \
	} while (0)

#define OMEGA_IMPORT __declspec(dllimport)
#define OMEGA_EXPORT __declspec(dllexport)

#define OMEGA_HAS_INT16_T
#define OMEGA_HAS_UINT16_T
#define OMEGA_HAS_INT32_T
#define OMEGA_HAS_UINT32_T
#define OMEGA_HAS_INT64_T
#define OMEGA_HAS_UINT64_T

#define OMEGA_UNUSED_ARG(n)	(void)(n)

#define OMEGA_COMPILER_STRING_I(a,b,c,d) a#b#c#d
#define OMEGA_COMPILER_STRING            OMEGA_COMPILER_STRING_I("gcc",__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__)

// These are missing from mingGW
enum {
    ASSOCF_INIT_NOREMAPCLSID           = 0x00000001,  //  do not remap clsids to progids
    ASSOCF_INIT_BYEXENAME              = 0x00000002,  //  executable is being passed in
    ASSOCF_OPEN_BYEXENAME              = 0x00000002,  //  executable is being passed in
    ASSOCF_INIT_DEFAULTTOSTAR          = 0x00000004,  //  treat "*" as the BaseClass
    ASSOCF_INIT_DEFAULTTOFOLDER        = 0x00000008,  //  treat "Folder" as the BaseClass
    ASSOCF_NOUSERSETTINGS              = 0x00000010,  //  dont use HKCU
    ASSOCF_NOTRUNCATE                  = 0x00000020,  //  dont truncate the return string
    ASSOCF_VERIFY                      = 0x00000040,  //  verify data is accurate (DISK HITS)
    ASSOCF_REMAPRUNDLL                 = 0x00000080,  //  actually gets info about rundlls target if applicable
    ASSOCF_NOFIXUPS                    = 0x00000100,  //  attempt to fix errors if found
    ASSOCF_IGNOREBASECLASS             = 0x00000200,  //  dont recurse into the baseclass
};

#endif // OOCORE_CONFIG_GCC_H_INCLUDED_
