///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
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

#ifndef OOCORE_CONFIG_BUILD_H_INCLUDED_
#define OOCORE_CONFIG_BUILD_H_INCLUDED_

// Testing!
//#define HAVE_CONFIG_H 1

/////////////////////////////////////////////////////////
//
// This file tries to guess the build tool you are using.
//
// It will error if it can't work it out
// Please contact the omegaonline team if it happens
//
/////////////////////////////////////////////////////////

#if defined(HAVE_CONFIG_H)
// Autoconf
#include "config-autoconf.h"
#elif defined(_MSC_VER)
// MS Visual Studio
#include "config-vstudio.h"
#elif defined(CODEBLOCKS)
// Code::Blocks
#include "config-codeblocks.h"
#elif defined(XCODE)
// XCode
#error TODO!
#elif defined (__BORLANDC__)
// Borland C++ Builder
#include <OOCore/config-borland.h>
#else
#error Failed to guess your system.  Please contact the developers.
#endif

#endif // OOCORE_CONFIG_BUILD_H_INCLUDED_
