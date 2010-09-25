#!/bin/sh

cd ../src/OOCore
echo Starting ooserverd...

# Kill any running ooserverd

rm /tmp/omegaonline &> /dev/null
rm /tmp/oo-* &> /dev/null

# This forces libtool to link the correct dlls under Win32...
../OOServer/ooserverd --version > /dev/null
../OOServer/oosvruser --version > /dev/null

if test "$OMEGA_DEBUG" = "yes" && test -n "$DISPLAY"; then
	xterm -e ../OOServer/ooserverd --unsafe -f $PWD/../$srcdir/test.conf &
else
	../OOServer/ooserverd --unsafe -f $PWD/../$srcdir/test.conf &
fi

child=$!
echo Giving ooserverd a chance to start...
sleep 3s

# Start up oosvruser
if test -f ../../tools/OOLaunch/oo-launch; then
	eval `../../tools/OOLaunch/oo-launch`
	#echo "OMEGA_SESSION_ADDRESS=$OMEGA_SESSION_ADDRESS"
	#echo "OMEGA_SESSION_PID=$OMEGA_SESSION_PID"
fi

echo Running tests...
echo
cd ../../test

../libtool --mode=execute -dlopen ../src/OOServer/oosvrlite.la -dlopen TestLibrary/testlibrary.la ./coretests
ret=$?

#kill $child &> /dev/null
kill -9 $child

# Make sure our session is dead...
kill -9 $OMEGA_SESSION_PID &> /dev/null

exit $ret
