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
build: .phony $(BUILD_DIR)/$(TARGET)
$(BUILD_DIR)/$(TARGET): depend $(OBJS)
	@$(ECHO) "\nLinking " "$(shell basename $(TARGET))" ;
ifdef BUILDING_LIB
	@$(CC) -shared $(OBJS) -Wl $(LDFLAGS) -o $(BUILD_DIR)/$(TARGET) 2>> $(BUILD_LOG)
else
	@$(CC) $(OBJS) $(LDFLAGS) -o $(BUILD_DIR)/$(TARGET) 2>> $(BUILD_LOG)
endif
