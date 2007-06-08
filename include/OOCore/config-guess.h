#ifndef OOCORE_CONFIG_GUESS_H_INCLUDED_
#define OOCORE_CONFIG_GUESS_H_INCLUDED_

#if defined(WIN32) || defined(_WIN32) || defined (__WIN32) || defined(__WIN32__)
#include <OOCore/config-win32.h>
#elif
// We assume we are using some kind of automake...
#endif

#endif // OOCORE_CONFIG_GUESS_H_INCLUDED_
