///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

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
