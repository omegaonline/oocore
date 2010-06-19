#!/bin/sh

cd ../src/OOCore
echo Starting OOServer...

# This forces libtool to link the correct dlls...
../OOServer/ooserverd --version
../OOServer/oosvruser --version


../OOServer/ooserverd --unsafe -f ../$srcdir/CoreTests/test.conf &
child=$!
sleep 3s
cd ../../test

../libtool --mode=execute -dlopen ../src/OOServer/oosvrlite.la -dlopen CoreTests/TestLibrary/testlibrary.la ./CoreTests/coretests
ret=$?

kill $child &> /dev/null

exit $ret
