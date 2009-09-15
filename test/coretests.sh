#!/bin/sh
cp $srcdir/ooregister.sh ./ooregister
cp $srcdir/ooregister.bat .

if test -x $srcdir/../bin/Debug/TestLibrary_msvc.dll; then
	ln -s $srcdir/../bin/Debug/TestLibrary_msvc.dll
fi

ln -s $srcdir/CoreTests/UTF-8-test.txt
ln -s $srcdir/CoreTests/UTF-16-test.txt

libtool --mode=execute -dlopen ../src/OOServer/oosvrlite.la -dlopen CoreTests/TestLibrary/testlibrary.la ./CoreTests/coretests
