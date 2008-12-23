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

# These override the implict pattern rules for source compilation
# to allow for OBJ_DIR to change depending on the build configuration.
# 
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.cpp
	@ $(ECHO) "\tBuilding [$<]" ;
	@ $(CXX) -c $< $(CPPFLAGS) -o $@ 

$(OBJ_DIR)/%.o : $(TEST_DIR)/%.cpp
	@ $(ECHO) "\tBuilding [$<]" ;
	@ $(CXX) -c $< $(TEST_CPPFLAGS) -o  $@ 

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@ $(ECHO) "\tBuilding [$<]" ;
	@ $(CC) -c $< $(CFLAGS) -o $@ 

$(OBJ_DIR)/%.o : $(TEST_DIR)/%.c
	@ $(ECHO) "\tBuilding [$<]" ;
	@ $(CC) -c $< $(TEST_CFLAGS) -o  $@
