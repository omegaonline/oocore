#!/bin/sh
cp $srcdir/ooregister.sh ./ooregister
cp $srcdir/ooregister.bat .

if test -x $srcdir/../bin/Debug/TestLibrary.dll; then
	ln -s $srcdir/../bin/Debug/TestLibrary.dll
fi

libtool --mode=execute -dlopen ../src/OOServer/oosvrlite.la ./CoreTests/coretests
