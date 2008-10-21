#///////////////////////////////////////////////////////////////////////////////////
#//
#// Copyright (C) 2008 Jamal M. Natour
#//
#// This file is part of the OmegaOnline  package.
#//
#//  is free software: you can redistribute it and/or modify
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

include ./makefiles/OOBuildEnv.mk
USE_LIB_OOCORE	:= 1
EXTRA_LIBS	:= -lACE -lsqlite3 -lmd5
TARGET		:= OOServerd
SRC_DIR 	:= src/OOServer
#SRCS         	:= $(filter-out UserMain.cpp ,$(ALL_CPP_SRCS))
SRCS         	:= 	$(SRC_DIR)/Channel.cpp			\
			$(SRC_DIR)/Database.cpp			\
			$(SRC_DIR)/MessageConnection.cpp	\
			$(SRC_DIR)/MessagePipeUnix.cpp		\
			$(SRC_DIR)/NTService.cpp		\
			$(SRC_DIR)/RegistryHive.cpp		\
			$(SRC_DIR)/RootHttp.cpp			\
			$(SRC_DIR)/RootMain.cpp			\
			$(SRC_DIR)/RootManager.cpp		\
			$(SRC_DIR)/RootRegistry.cpp		\
			$(SRC_DIR)/SpawnedProcessUnix.cpp
include ./makefiles/OOBuildTargets.mk

