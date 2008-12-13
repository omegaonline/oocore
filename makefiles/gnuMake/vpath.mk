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

# Vpath declarations all vars are declared [ insert filename ]

# c files are found under SRC_DIR
vpath %.c 	$(SRC_DIR)

# cpp files are found under SRC_DIR
vpath %.cpp 	$(SRC_DIR)
    
# object files and dependencies are found under OBJ_DIR which resolves to 
# BUILD_DIR/Debug/objs_debug for DEBUG builds [default] 
# BUILD_DIR/objs_release for RELEASE builds
vpath %.o 	$(OBJ_DIR)
vpath %$(DEPS) 	$(OBJ_DIR)

# shared object files (libs) are found under BUILD_DIR
vpath %.so 	$(BUILD_DIR)
