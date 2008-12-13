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

# building lib
INSTALL_DIR  := $(INSTALL_LIB_ROOT)
ifndef RELEASE
TARGET	:= $(subst .so,d.so,$(TARGET))
endif
TEST_LDFLAGS := -L$(OBJ_DIR) -L$(BUILD_DIR) -l$(subst lib,,$(subst .so,,$(TARGET)))
CFLAGS := -fPIC $(CFLAGS)
CPPFLAGS := -fPIC $(CPPFLAGS)

