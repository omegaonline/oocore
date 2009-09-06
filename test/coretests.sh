#!/bin/sh
cp $srcdir/ooregister.sh ./ooregister
cp $srcdir/ooregister.bat .

if test -x $srcdir/../bin/Debug/TestLibrary_msvc.dll; then
	ln -s $srcdir/../bin/Debug/TestLibrary_msvc.dll
fi

libtool --mode=execute -dlopen ../src/OOServer/oosvrlite.la -dlopen CoreTests/TestLibrary/testlibrary.la ./CoreTests/coretests
