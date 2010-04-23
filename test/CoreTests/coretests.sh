#!/bin/bash
ln -s -f $srcdir/ooregister.sh ./ooregister
ln -s -f $srcdir/ooregister.bat .

cd ../src/OOCore
ln -s -f ../$srcdir/CoreTests/test.conf .
echo Starting OOServer...
../OOServer/ooserverd --unsafe --batch -f test.conf &
child=$!
sleep 3s
cd ../../test

../libtool --mode=execute -dlopen ../src/OOServer/oosvrlite.la -dlopen CoreTests/TestLibrary/testlibrary.la ./CoreTests/coretests
ret=$?

kill $child &> /dev/null

exit $ret
