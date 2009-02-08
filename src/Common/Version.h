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

#ifndef OOCORE_VERSION_H_INCLUDED_
#define OOCORE_VERSION_H_INCLUDED_

//////////////////////////////////////////////
// Version defines

#define OOCORE_MAJOR_VERSION  0
#define OOCORE_MINOR_VERSION  4
#define OOCORE_PATCH_VERSION  4

#define OOCORE_VERSION_III(n)        #n
#define OOCORE_VERSION_II(a,b,c)     OOCORE_VERSION_III(a.b.c)
#define OOCORE_VERSION_I(a,b,c)      OOCORE_VERSION_II(a,b,c)
#define OOCORE_VERSION               OOCORE_VERSION_I(OOCORE_MAJOR_VERSION,OOCORE_MINOR_VERSION,OOCORE_PATCH_VERSION)

#endif // OOCORE_VERSION_H_INCLUDED_
