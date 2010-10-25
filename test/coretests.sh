#!/bin/sh

cd ../src/OOCore
echo Starting ooserverd...

# Kill any running ooserverd

# This forces libtool to link the correct dlls under Win32...
../OOServer/ooserverd --version > /dev/null
../OOServer/oosvruser --version > /dev/null

if test "$OMEGA_DEBUG" = "yes" && test -n "$DISPLAY"; then
	xterm -e ../OOServer/ooserverd --unsafe --conf-file $PWD/../$srcdir/test.conf --pidfile ../../test/ooserverd.pid &
else
	../OOServer/ooserverd --unsafe --conf-file $PWD/../$srcdir/test.conf --pidfile ../../test/ooserverd.pid &
fi

echo Giving ooserverd a chance to start...
sleep 3s

# Start up oosvruser
if test -f ../../tools/OOLaunch/oo-launch; then
	eval `../../tools/OOLaunch/oo-launch`
fi

echo Running tests...
echo
cd ../../test

../libtool --mode=execute -dlopen ../src/OOServer/oosvrlite.la -dlopen TestLibrary/testlibrary.la ./coretests
ret=$?

# Close our ooserver
if test -f ./ooserverd.pid; then
	pid=$(cat "./ooserverd.pid") 
	kill $pid
fi

exit $ret
