#!/bin/sh

cd ../src/OOCore

# This forces libtool to link the correct dlls under Win32...
../OOServer/ooserverd --version
../OOServer/oosvruser --version

server_launch="../OOServer/ooserverd"

if test "x$OMEGA_DEBUG" = "xyes" && test -n "$TERM"; then
	#Try to do something sensible with $TERM
	case $TERM in
		cygwin)
			# Mingw needs a load of help here...
			server_launch="$COMSPEC //Q //c start ../OOServer/ooserverd.exe"
			;;
		*)
			server_launch="$TERM -e ../OOServer/ooserverd"
			;;
	esac
fi

$server_launch --unsafe --conf-file $PWD/../$srcdir/test.conf --pidfile ../../test/ooserverd.pid &

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
