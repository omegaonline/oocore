#!/bin/sh

cd ../src/OOCore
echo Starting OOServer...

# This forces libtool to link the correct dlls...
../OOServer/ooserverd --version
../OOServer/oosvruser --version

../OOServer/ooserverd --unsafe -f ../$srcdir/test.conf &
child=$!
sleep 5s

# Start up oosvruser
if test -f ../OOServer/oo-launch; then
	set OMEGA_USER_BINARY ../OOServer/oosvruser
	eval `../OOServer/oo-launch`
	echo "OMEGA_SESSION_ADDRESS=$OMEGA_SESSION_ADDRESS"
fi

cd ../../test

../libtool --mode=execute -dlopen ../src/OOServer/oosvrlite.la -dlopen TestLibrary/testlibrary.la ./coretests
ret=$?

kill $child &> /dev/null

exit $ret
