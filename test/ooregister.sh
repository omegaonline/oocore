#!/bin/sh
libtool --mode=execute -dlopen ../src/OOServer/oosvrlite.la -dlopen CoreTests/TestLibrary/testlibrary.la ../src/OORegister/ooregister $@
