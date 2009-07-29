#!/bin/sh
libtool --mode=execute -dlopen ../src/OOServer/oosvrlite.la ../src/OORegister/ooregister $@
