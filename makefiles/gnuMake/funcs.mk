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

define die_unless_list
	@ $(if $(strip $($1)), $(ECHO) "$(1)"$(foreach target,$($(1)), "\n\t"$(target)), $(ECHO) "Error: \t$(1) is unset" && false )
endef

define die_unless_set
	@ $(if $(strip $($1)), $(ECHO) "$(1)\t"$(foreach target,$($(1)), "=\t"$(target)), $(ECHO) "Error: \t$(1) is unset" && false )
endef

define print_srcs_list
    	@$(call die_unless_list,TEST_LDFLAGS)
	@$(call die_unless_list,LDFLAGS)
	@$(call die_unless_list,SRCS)
	@$(call die_unless_list,OBJS)
	@$(call die_unless_list,CFLAGS)
	@$(call die_unless_list,CPPFLAGS)
endef

define check_for_internal_vars
	@$(call die_unless_set,OBJ_DIR)
	@$(call die_unless_set,TARGET)
	@$(call die_unless_set,DEPS)
	@$(call die_unless_set,BUILD_LOG)
	@$(call die_unless_set,INSTALL_DIR)
	@$(call die_unless_set,INSTALL_LIB_ROOT)
	@$(call die_unless_set,INSTALL_HDR_ROOT)
	@$(call die_unless_set,LD_LIBRARY_PATH)
	@$(call die_unless_set,BUILD_DIR)
	@$(call die_unless_set,SRC_DIR)
endef

define print_build_version
	@ $(ECHO) "$(BUILD_TYPE) on :" $(shell $(DATE) +"%F  @ %T ")
	@ $(ECHO) "Version: $(BUILD_DATE)" 
	$(call make_dir BUILD_DIR)
endef

