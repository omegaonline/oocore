#! /bin/sh

aclocal -I m4 && \
libtoolize --copy && \
automake --foreign --add-missing --copy && \
autoconf
