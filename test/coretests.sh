#!/bin/sh
if ! test -x ./ooregister; then
	cp $srcdir/ooregister.sh ./ooregister
fi
if ! test -x ./ooregister.bat; then
	cp $srcdir/ooregister.bat .
fi
libtool --mode=execute -dlopen ../src/OOServer/oosvrlite.la ./CoreTests/coretests
