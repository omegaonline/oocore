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

# These flags are set here to be sure all substitutions take place before targets are evaluated
CFLAGS	     := $(CFLAGS) $(BUILD_DEFS)
CPPFLAG	     := $(CPPFLAGS) $(BUILD_DEFS)
OBJS 	     := $(subst $(SRC_DIR), $(OBJ_DIR),$(subst .c,.o,$(subst .cpp,.o, $(SRCS))))
TEST_OBJS    :=	$(subst $(TEST_DIR), $(OBJ_DIR),$(subst .c,.o, $(TEST_SRCS)))
TEST_TARGETS :=	$(subst $(TEST_DIR), $(BUILD_DIR),$(subst .c,, $(TEST_SRCS)))
TEST_CFLAGS  := $(TEST_CFLAGS) -g $(TEST_DEFS)
TEST_CPPFLAGS:= $(TEST_CPPFLAGS) -g $(TEST_DEFS)
LDFLAGS	     := -L$(OBJ_DIR) -L$(BUILD_DIR) -L$(INSTALL_LIB_ROOT) $(EXTRA_LIBS) $(LDFLAGS)
DEP_FLAGS    := BUILD_DIR=$(BUILD_DIR) RELEASE=1

# These is the target run when make is run without additional arguments
first: depend build

# These are composite targets to save on much typing
all:	clean depend build test
tall:	all run-test

# useful functions shared by sub makefiles
include $(ENV_DIR)/funcs.mk


# Pull in all the sub targets under the make specific targets directory,
# currently gnuMake/ but could be bsdMake or another make system.
#  e
include  $(ENV_DIR)/targets/*.mk


