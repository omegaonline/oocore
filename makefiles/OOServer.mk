#///////////////////////////////////////////////////////////////////////////////////
#//
#// Copyright (C) 2008 Jamal M. Natour
#//
#// This file is part of the OmegaOnline  package.
#//
#// OmegaOnline is free software: you can redistribute it and/or modify
#// it under the terms of the GNU Lesser General Public License as published by
#// the Free Software Foundation, either version 3 of the License, or
#// (at your option) any later version.
#//
#// OmegaOnline is distributed in the hope that it will be useful,
#// but WITHOUT ANY WARRANTY; without even the implied warranty of
#// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#// GNU Lesser General Public License for more details.
#//
#// You should have received a copy of the GNU Lesser General Public License
#// along with this file.  If not, see <http://www.gnu.org/licenses/>.
#//
#///////////////////////////////////////////////////////////////////////////////////

include $(ENV_DIR)/makefile.env
USE_LIB_OOCORE	:= 1
TARGET		:= OOServerd
SRC_DIR 	:= $(SRC_DIR)/OOServer

SRCS         	:= 	$(SRC_DIR)/Database.cpp			\
			$(SRC_DIR)/MessageConnection.cpp	\
			$(SRC_DIR)/MessagePipeUnix.cpp		\
			$(SRC_DIR)/NTService.cpp		\
			$(SRC_DIR)/RegistryHive.cpp		\
			$(SRC_DIR)/RootHttp.cpp			\
			$(SRC_DIR)/RootMain.cpp			\
			$(SRC_DIR)/RootManager.cpp		\
			$(SRC_DIR)/RootRegistry.cpp		\
			$(SRC_DIR)/SpawnedProcessUnix.cpp

include $(ENV_DIR)/makefile.targets
