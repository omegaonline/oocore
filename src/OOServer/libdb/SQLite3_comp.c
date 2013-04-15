///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2012 Rick Taylor
//
// This file is part of OOCore/libdb, the Omega Online Core db library.
//
// OOCore/libdb is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore/libdb is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore/libdb.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
//
// This file is purely a hack to stop us having to directly reference the SQLite3 
// installation
//
///////////////////////////////////////////////////////////////////////////////////

#include "../../oocore-config.h"

#ifdef HAVE_SQLITE3_AMALGAMATION

#if defined(HAVE_PTHREAD) && !defined(PTHREAD_MUTEX_RECURSIVE)
#define SQLITE_HOMEGROWN_RECURSIVE_MUTEX
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4127 4244 4132 4706 4306)

#if (_MSC_VER >= 1600) && defined(CODE_ANALYSIS)
#include <codeanalysis\warnings.h>
#pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )
#endif
#endif

#include <sqlite3.c>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif
