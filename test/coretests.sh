#!/bin/sh

cd ../src/OOCore
echo Starting OOServer...

# This forces libtool to link the correct dlls...
../OOServer/ooserverd --version
../OOServer/oosvruser --version

../OOServer/ooserverd --unsafe -f $PWD/../$srcdir/test.conf &
child=$!
sleep 5s

# Start up oosvruser
if test -f ../../tools/OOLaunch/oo-launch; then
	OMEGA_USER_BINARY=$PWD/../OOServer/oosvruser
	echo "OMEGA_USER_BINARY=$OMEGA_USER_BINARY"
	export OMEGA_USER_BINARY
	eval `../../tools/OOLaunch/oo-launch`
	echo "OMEGA_SESSION_ADDRESS=$OMEGA_SESSION_ADDRESS"
	echo "OMEGA_SESSION_PID=$OMEGA_SESSION_PID"
fi

cd ../../test

../libtool --mode=execute -dlopen ../src/OOServer/oosvrlite.la -dlopen TestLibrary/testlibrary.la ./coretests
ret=$?

#kill $child &> /dev/null
kill -9 $child

exit $ret
