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
include makefiles/OOBuildEnv.mk
SRC_DIR	     	:= src/OOCore
TARGET		:= libOOCore.so
EXTRA_LIBS	:= -lACE -lsqlite3 -lmd5

# todo figure out how to infer this from the sources
OOCORE_INSTALL	:= 1
# todo figure out how to infer this from the target
BUILDING_LIB	:= 1

PRIVATE_HDRS	:= $(SRC_DIR)/OOCore_precomp.h
SRCS	     	:= $(ALL_CPP_SRCS)
#boilerplate targets
include makefiles/OOBuildTargets.mk