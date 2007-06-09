#ifndef OOCORE_CONFIG_GUESS_H_INCLUDED_
#define OOCORE_CONFIG_GUESS_H_INCLUDED_

#if defined(WIN32) || defined(_WIN32) || defined (__WIN32) || defined(__WIN32__)
#include <OOCore/config-win32.h>
#elif defined(__unix) || defined(__unix__)
#include <OOCore/config-unix.h>
#else
// We assume we are using some kind of automake...
#error TODO!
#endif

#endif // OOCORE_CONFIG_GUESS_H_INCLUDED_
