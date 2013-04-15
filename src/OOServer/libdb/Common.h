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

#ifndef OOCORE_LIBDB_COMMON_H_INCLUDED_
#define OOCORE_LIBDB_COMMON_H_INCLUDED_

#include <OOBase/SmartPtr.h>
#include <OOBase/String.h>
#include <OOBase/Vector.h>
#include <OOBase/Set.h>
#include <OOBase/Timeout.h>
#include <OOBase/Mutex.h>

#if !defined(_MSC_VER)
#include <oocore-autoconf.h>
#endif

#if defined(_WIN32)
	// Remove the unistd include - we are windows
	#if defined(HAVE_UNISTD_H)
	#undef HAVE_UNISTD_H
	#endif
#endif

#include "../../include/Omega/internal/base_types.h"

#endif // OOCORE_LIBDB_COMMON_H_INCLUDED_
