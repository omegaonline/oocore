#!/bin/sh
if ! test -x ./ooregister; then
	cp $srcdir/ooregister.sh ./ooregister
fi
libtool --mode=execute -dlopen ../src/OOServer/oosvrlite.la ./CoreTests/coretests
