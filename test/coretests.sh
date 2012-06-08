#!/bin/sh

cd ../src/OOCore

# This forces libtool to link the correct dlls under Win32...
../OOServer/ooserverd --version
../OOServer/oosvruser --version

server_launch="../OOServer/ooserverd"

if test -n "$TERM"; then
	#Try to do something sensible with $TERM
	case $TERM in
		cygwin)
			# MSYS needs a load of help here...
			cp .libs/OOCore32.dll .
			server_launch="$COMSPEC //Q //c start ../OOServer/.libs/ooserverd.exe"
			;;
		*)
			if test -n "$DISPLAY"; then
				server_launch="$TERM -e $server_launch"
				#server_launch="$TERM -e libtool --mode=execute valgrind --tool=callgrind $server_launch" 
			fi
			;;
	esac
fi

$server_launch --debug --conf-file $PWD/../$srcdir/test.conf --pidfile ../../test/ooserverd.pid &

echo Giving ooserverd a chance to start...
sleep 3s

echo Running tests...
echo
cd ../../test

../libtool --mode=execute -dlopen TestLibrary/testlibrary.la ./coretests
ret=$?

# Close our ooserver
if test -f ./ooserverd.pid; then
	kill `cat ./ooserverd.pid`
fi

exit $ret
