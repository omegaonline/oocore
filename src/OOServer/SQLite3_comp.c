///////////////////////////////////////////////////////////////////////////////////
//
// This file is purely a hack to stop us having to directly reference the SQLite3 
// installation
//
///////////////////////////////////////////////////////////////////////////////////

#include "../OOBase/config-base.h"

#ifdef HAVE_SQLITE3_AMALGAMATION
#include <sqlite3.c>
#endif
