#! /bin/sh
echo Running autotools...
aclocal -I m4 && \
autoheader && \
libtoolize --copy --ltdl --quiet && \
automake --foreign --add-missing --copy && \
autoconf
