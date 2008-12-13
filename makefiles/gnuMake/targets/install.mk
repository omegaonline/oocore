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
# INSTALL TARGET (requires root access)
# 1) copy target to final destination
# 2) chmod target to final permisions
install: .phony build
	@ $(ECHO) "entering install"
	! ( $(INSTALL) $(INSTALL_PERM) $(BUILD_DIR)/$(TARGET) $(INSTALL_DIR) ) || $(ECHO) "$(TARGET) is installed";
ifdef BUILDING_LIB	
ifdef OOCORE_INSTALL
	$(MKDIR) $(INSTALL_HDR_DIR)/OOCore
	$(MKDIR) $(INSTALL_HDR_DIR)/OTL
	! ( $(INSTALL) $(INSTALL_PERM) $(wildcard $(BUILD_ROOT)/include/OOCore/*.h) $(INSTALL_HDR_ROOT)/OOCore ) || $(ECHO) "OOCore headers installed"
	! ( $(INSTALL) $(INSTALL_PERM) $(wildcard $(BUILD_ROOT)/include/OTL/*.h) $(INSTALL_HDR_ROOT)/OTL ) || $(ECHO) "OTL headers installed"
endif	
ifdef HDRS
	$(call die_unless_list HDRS)
	! ( $(INSTALL) $(INSTALL_PERM) $(HDRS) $(INSTALL_HDR_ROOT) ) || $(ECHO) "Other headers installed"
endif
endif
	@ $(ECHO) "leaving  install\n"
