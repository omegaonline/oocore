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
work_dir_set()
{
	if [ $OMEGA_WORK_DIR"" == "" ] ;
	then
		echo "OMEGA_WORK_DIR is unset" ;
		return `false`;
	fi
	if [ $ENV_DIR"" == "" ] ;
	then
		echo "ENV_DIR is unset" ;
		return `false`;
	fi
	echo	"OMEGA_WORK_DIR=[$OMEGA_WORK_DIR]" ;
	echo	"ENV_DIR=[$ENV_DIR]" ;
	return `true`
}

make_oocore()
{
	{ cd $OMEGA_WORK_DIR; make -f makefiles/OOCore.mk $ENV_DIR "$@"; }
}

make_ooserver()
{
	{ cd $OMEGA_WORK_DIR; make -f makefiles/OOServer.mk $ENV_DIR "$@"; }
}
make_oouser()
{
	{ cd $OMEGA_WORK_DIR; make -f makefiles/OOSvrUsr.mk $ENV_DIR "$@" ; }
}

# See how we were called.
work_dir_set
case $1 in
	# build all the sub projects
	release-all)
		shift 1;
		make_oocore  all RELEASE=1 $*
		make_ooserver all RELEASE=1 $*
		make_oouser all RELEASE=1 $*
		exit $?;
		;;
	debug-all|all)
		shift 1;
		make_oocore   VERBOSE=1 $*
		make_ooserver VERBOSE=1 $*
		make_oouser   VERBOSE=1 $*
		exit $?;
		;;
	clean)
		shift 1;
		make_oocore    clean
		make_ooserver  clean
		make_oouser  distclean
		exit $?;
		;;
	# build the core
	oocore)
		shift 1;
	  	make_oocore $*
		exit $?;
		;;
	# build the server process
	ooserver)
  		shift 1;
	  	make_ooserver $*
		exit $?;
		;;
	# build the sandbox process
	oouser)
  		shift 1;
	  	make_oouser $*
		exit $?;
		;;
	# default:
	*)
		echo $"Usage: `basename $0` { release-all | all | oocore | ooserver | oouser | clean }";

		echo -e  "'build_omega.sh all' to build all modules."
		echo -e  "'build_omega.sh clean' to remove *all* autogenerated files."
		echo -e  "'build_omega.sh oocore' to build the core."
		echo -e  "'build_omega.sh ooserver' to build the server."
		echo -e  "'build_omega.sh oouser' to build the server user."
		false;
esac
