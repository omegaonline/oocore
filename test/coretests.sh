#!/bin/sh
cp $srcdir/ooregister.sh ./ooregister
cp $srcdir/ooregister.bat .

cp $srcdir/CoreTests/TestProcess/testprocess.sh ./CoreTests/TestProcess/testprocess.sh

if test -x $srcdir/../bin/Debug/TestLibrary_msvc.dll; then
	ln -s -f $srcdir/../bin/Debug/TestLibrary_msvc.dll
fi

if test -x $srcdir/../bin/Debug/TestProcess_msvc.exe; then
	ln -s -f $srcdir/../bin/Debug/TestProcess_msvc.exe
fi

ln -s -f $srcdir/CoreTests/UTF-8-test.txt
ln -s -f $srcdir/CoreTests/UTF-16-test.txt

../libtool --mode=execute -dlopen ../src/OOServer/oosvrlite.la -dlopen CoreTests/TestLibrary/testlibrary.la ./CoreTests/coretests
