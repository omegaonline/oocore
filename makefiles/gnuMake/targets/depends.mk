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

depend:
# DEPENDENCY GENERATION TARGET
	@ $(ECHO) "Entering depend\n"
ifdef VERBOSE
	@$(call print_build_version)
endif
	@ $(ECHO) "Checking for needed variables"
	@$(call check_for_internal_vars)
ifdef VERBOSE
	@$(call print_srcs_list)
ifdef TEST_TARGET
	@$(call die_unless_set,TEST_TARGET)
endif
endif
	@ $(ECHO) "Found needed variables\n"

# create build directories
	@ [ -d $(OBJ_DIR) ] || $(MKDIR) $(OBJ_DIR) && $(ECHO) "created $(OBJ_DIR)"

# 1) actual generation of dependency listing
	@ $(ECHO) "Generating Dependencies for $(BUILD_DIR)/$(TARGET)"
	@ $(RM) $(DEPS)
	@ $(TOUCH) $(DEPS)
	@ $(ifneq "" $(strip $(filter %.c, $(SRCS))) $(CC) -MM $(CFLAGS) $(C_SRCS) $(PRIVATE_HDRS)  $(HDRS) $(C_TEST_SRCS) >> $(DEPS) 2>> $(BUILD_LOG))
	@ $(ifneq "" $(strip $(filter %.cpp, $(SRCS))) $(CXX) -MM $(CPPFLAGS) $(CPP_SRCS) $(PRIVATE_HDRS)  $(HDRS) $(CPP_TEST_SRCS) >> $(DEPS)  2>> $(BUILD_LOG))
	@ $(ECHO) "leaving  depend\n"

