#///////////////////////////////////////////////////////////////////////////////////
#//
#// Copyright (C) 2008 Jamal M. Natour
#//
#// This file is part of the OmegaOnline  package.
#//
#// It is free software: you can redistribute it and/or modify
#// it under the terms of the GNU Lesser General Public License as published by
#// the Free Software Foundation, either version 3 of the License, or
#// (at your option) any later version.
#//
#//  is distributed in the hope that it will be useful,
#// but WITHOUT ANY WARRANTY; without even the implied warranty of
#// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#// GNU Lesser General Public License for more details.
#//
#// You should have received a copy of the GNU Lesser General Public License
#// along with this file.  If not, see <http://www.gnu.org/licenses/>.
#//
#///////////////////////////////////////////////////////////////////////////////////

#	Supported short cut flags
#
# 	USE_LIB_UTIL	link against libUtility
#	USE_LIB_STATE	link against libState
#	USE_LIB_PARSER	link against libParser
#	USE_LIB_OOCORE	link against libOOCore

# build against the release version of libUtility in RELEASE builds
# build against the debug version of libUtility in DEBUG builds
ifdef USE_LIB_UTIL
ifndef RELEASE
LDFLAGS := $(LDFLAGS) -lUtilityd
else
LDFLAGS := $(LDFLAGS) -lUtility
endif
endif

# build against the release version of libState in RELEASE builds
# build against the debug version of libState in DEBUG builds
ifdef USE_LIB_STATE
ifndef RELEASE
LDFLAGS := $(LDFLAGS) -lStated
else
LDFLAGS := $(LDFLAGS) -lState
endif
endif

# build against the release version of libParser in RELEASE builds
# build against the debug version of libParser in DEBUG builds
ifdef USE_LIB_PARSER
ifndef RELEASE
LDFLAGS := $(LDFLAGS) -lParserd
else
LDFLAGS := $(LDFLAGS) -lParser
endif
endif


# build against the release version of libOOCore in RELEASE builds
# build against the debug version of libOOCore in DEBUG builds
ifdef USE_LIB_OOCORE
ifndef RELEASE
LDFLAGS := $(LDFLAGS) -lOOCored
else
LDFLAGS := $(LDFLAGS) -lOOCore
endif
endif


