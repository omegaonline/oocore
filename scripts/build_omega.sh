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
	echo	"OMEGA_WORK_DIR=[$OMEGA_WORK_DIR]" ;
	return `true`
}

make_oocore()
{
	{ cd $OMEGA_WORK_DIR; make -f makefiles/OOCore.mk "$@"; }
}

make_ooserver()
{
	{ cd $OMEGA_WORK_DIR; make -f makefiles/OOServer.mk "$@"; }
}
make_oouser()
{
	{ cd $OMEGA_WORK_DIR; make -f makefiles/OOSrvUsr.mk "$@"; }
}

# See how we were called.
work_dir_set
case $1 in
	# build all the sub projects
	all)
		shift 1;
		make_oocore   $*
		make_ooserver $*
		make_oouser $*
		exit $?;
		;;
	clean)
		shift 1;
		make_oocore    clean
		make_ooserver  clean
		make_oouser  clean
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
		echo $"Usage: `basename $0` { all | oocore | ooserver | oouser | clean }";
		false;
esac
