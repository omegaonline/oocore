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
#!/bin/sh
export OMEGA_WORK_DIR=`pwd`
export ENV_DIR="$OMEGA_WORK_DIR/makefiles/gnuMake"
export PATH=$PATH:$OMEGA_WORK_DIR/scripts

echo "OMEGA_WORK_DIR=$OMEGA_WORK_DIR";
echo "ENV_DIR=$ENV_DIR";

 echo -e  "\ttype 'build_omega.sh all' to build all modules."
 echo -e  "\ttype 'build_omega.sh clean' to remove all autogenerated files"
 echo -e  "\ttype 'build_omega.sh oocore' to build the core."
 echo -e  "\ttype 'build_omega.sh ooserver' to build the server."
 echo -e  "\ttype 'build_omega.sh oouser' to build the server user."