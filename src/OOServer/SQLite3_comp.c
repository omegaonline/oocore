///////////////////////////////////////////////////////////////////////////////////
//
// This file is purely a hack to stop us having to directly reference the SQLite3 
// installation
//
///////////////////////////////////////////////////////////////////////////////////

#include "../OOBase/config-base.h"

#ifdef HAVE_SQLITE3_AMALGAMATION

#if defined(HAVE_PTHREAD) && !defined(PTHREAD_MUTEX_RECURSIVE)
#define SQLITE_HOMEGROWN_RECURSIVE_MUTEX
#endif

#include <sqlite3.c>
#endif
