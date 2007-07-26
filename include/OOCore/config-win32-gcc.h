#ifndef OOCORE_CONFIG_WIN32_GCC_H_INCLUDED_
#define OOCORE_CONFIG_WIN32_GCC_H_INCLUDED_

#include <OOCore/config-gcc.h>

// We use the unicode CRT
#define _UNICODE
#include <objbase.h>

#define ACE_LACKS_SYSTEM

#undef interface
#define interface struct

#define OMEGA_IMPORT __declspec(dllimport)
#define OMEGA_EXPORT __declspec(dllexport)

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

#endif // OOCORE_CONFIG_WIN32_GCC_H_INCLUDED_
