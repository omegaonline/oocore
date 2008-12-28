#///////////////////////////////////////////////////////////////////////////////////
#//
#// Copyright (C) 2008 Rick Taylor
#//
#// This file is part of the OmegaOnline  package.
#//
#// It is free software: you can redistribute it and/or modify
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

#!/bin/sh

# fix the no recreate after clean bug
TOP_SRC_DIR=. ENV_DIR=. scripts/build_omega.sh clean
[ -d ./build ] || mkdir ./build;

# this is now replacing some of makefile.env, 
# eventually this should replace almost all of it

cat<<EOF > build/configured_vars.mk.in
#///////////////////////////////////////////////////////////////////////////////////
#//
#// Copyright (C) 2008 Rick Taylor
#//
#// This file is part of the OmegaOnline  package.
#//
#// It is free software: you can redistribute it and/or modify
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

#generated by `basename $0` on `date`
@SET_MAKE@
# stuff to autoconf
CC		= @CC@
CXX		= @CXX@
MKDIR		= @MKDIR_P@
SED		= @SED@ -e
INSTALL		= @INSTALL@ -m
TOP_SRC_DIR	= @srcdir@
ENV_DIR		= @srcdir@/makefiles/@MAKE_STYLE@
EOF

# This is now replacing Makefile.in

cat<<EOF > Makefile.in
#///////////////////////////////////////////////////////////////////////////////////
#//
#// Copyright (C) 2008 Rick Taylor
#//
#// This file is part of the OmegaOnline  package.
#//
#// It is free software: you can redistribute it and/or modify
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

#generated by `basename $0` on `date`
include @srcdir@/build/configured_vars.mk
@SET_MAKE@
OPTS=TOP_SRC_DIR=\$(TOP_SRC_DIR) ENV_DIR=\$(ENV_DIR)
first: all

uninstall:
	@\$(OPTS) \$(TOP_SRC_DIR)/scripts/build_omega.sh uninstall
install:
	@\$(OPTS) \$(TOP_SRC_DIR)/scripts/build_omega.sh install 
all:
	@\$(OPTS) \$(TOP_SRC_DIR)/scripts/build_omega.sh -v oocore
	@\$(OPTS) \$(TOP_SRC_DIR)/scripts/build_omega.sh -v ooserver
	@\$(OPTS) \$(TOP_SRC_DIR)/scripts/build_omega.sh -v oouser
release:
	@\$(OPTS) \$(TOP_SRC_DIR)/scripts/build_omega.sh  -rv all 
depend:
	@\$(OPTS) \$(TOP_SRC_DIR)/scripts/build_omega.sh depend 
clean:
	@\$(OPTS) \$(TOP_SRC_DIR)/scripts/build_omega.sh clean
help:
	@\$(OPTS) \$(TOP_SRC_DIR)/scripts/build_omega.sh help
EOF

# This is now replacing configure.ac

cat<<EOF > configure.ac
#generated by `basename $0` on `date`
# Process this file with autoconf to produce a configure script

AC_INIT(OmegaOnline,0.4.2,www.omegaonline.org.uk)
AC_CONFIG_SRCDIR(src/OOCore/OOCore.cpp)
AC_CONFIG_AUX_DIR(build/build-aux)
# Try to work out which make to use
#AM_INIT_AUTOMAKE()
AC_CHECK_PROG(MAKE_STYLE,mingw32-make,gnuMake)
AC_CHECK_PROG(MAKE_STYLE,gmake,gnuMake)
AC_CHECK_PROG(MAKE_STYLE,make,unknownMake)

AC_PROG_MKDIR_P()
AC_PROG_CC()
AC_PROG_CXX()
AC_PROG_MAKE_SET()

# stuff to autoconf
AC_PROG_INSTALL()
AC_PROG_SED()

AC_CONFIG_FILES([Makefile build/configured_vars.mk])
AC_OUTPUT
EOF

autoreconf -i
# bugger automake
automake --add-missing  2> /dev/null
autoconf
