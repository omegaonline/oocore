#! /bin/sh

aclocal -I m4 && \
autoheader && \
libtoolize --copy && \
automake --foreign --add-missing --copy && \
autoconf
