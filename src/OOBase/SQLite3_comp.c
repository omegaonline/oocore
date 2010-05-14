///////////////////////////////////////////////////////////////////////////////////
//
// This file is purely a hack to stop us having to directly reference the SQLite3 
// installation
//
///////////////////////////////////////////////////////////////////////////////////

#include "config-base.h"

#ifdef HAVE_SQLITE3_AMALGAMATION

#if defined(HAVE_PTHREAD) && !defined(PTHREAD_MUTEX_RECURSIVE)
#define SQLITE_HOMEGROWN_RECURSIVE_MUTEX
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4127)
#pragma warning(disable:4244)
#endif

#include <sqlite3.c>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif
