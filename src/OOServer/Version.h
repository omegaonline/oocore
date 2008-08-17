///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOSERVER_VERSION_H_INCLUDED_
#define OOSERVER_VERSION_H_INCLUDED_

//////////////////////////////////////////////
// Version defines

#define OOSERVER_MAJOR_VERSION  0
#define OOSERVER_MINOR_VERSION  4
#define OOSERVER_BUILD_VERSION  3

#define OOSERVER_VERSION_II(a,b,c)	#a "." #b "." #c
#define OOSERVER_VERSION_I(a,b,c)	OOSERVER_VERSION_II(a,b,c)
#define OOSERVER_VERSION			OOSERVER_VERSION_I(OOSERVER_MAJOR_VERSION,OOSERVER_MINOR_VERSION,OOSERVER_BUILD_VERSION)


#endif // OOSERVER_VERSION_H_INCLUDED_
