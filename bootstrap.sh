#! /bin/sh
echo Running autotools...
aclocal -I m4 && \
autoheader && \
libtoolize --copy --ltdl --force && \
automake --foreign --add-missing --copy && \
autoconf
