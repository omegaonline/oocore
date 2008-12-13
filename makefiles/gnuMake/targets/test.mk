
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
ifdef TEST_SRCS	
run-test: test $(BUILD_DIR)/$(TARGET)
	@$(ECHO) "Running Tests" ;
	@LD_LIBRARY_PATH="$(BUILD_DIR):$(INSTALL_LIB_ROOT):$(LD_LIBRARY_PATH)" $(TEST_INVOKE)

test: .phony $(TEST_OBJS)
	@$(ECHO) "Linking Tests" ;
	@for f in $(subst .o,,$(TEST_OBJS)) ;						 \
	do										 \
		obj_file=`$(ECHO) $$f | $(SED) "s:\$(OBJ_DIR):\$(BUILD_DIR):" ` ; 	 \
		$(ECHO) "\tLinking [$$obj_file]" ;					 \
		$(CC) $(TEST_CFLAGS) $$f.o $(TEST_LDFLAGS) -o $$obj_file ;		 \
	done 
else
# turn these targets into no-ops
run-test: 	.phony
test:		.phony
endif # ndef TEST_SRCS

