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

# Uninstall
#
# Remove libs/binaries and headers installed by the install target - targets/install.mk
#
uninstall: .phony
	@ $(ECHO) "entering uninstall\n"
	$(RM) $(INSTALL_DIR)/$(TARGET)
ifdef OOCORE_INSTALL
	$(RM) -rvf $(INSTALL_HDR_DIR)/OOCore || $(ECHO) "OOCore headers removed"
	$(RM) -rvf $(INSTALL_HDR_DIR)/OTL || $(ECHO) "OOCore headers removed"
endif
ifdef BUILDING_LIB	
	@ $(if $(strip $(HDRS)),$(RM) -v $(INSTALL_HDR_ROOT)/$(HDRS),)
endif
	@ $(ECHO) "leaving  uninstall\n"


