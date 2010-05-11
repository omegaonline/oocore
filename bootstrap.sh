#! /bin/sh
echo Running autotools...
aclocal -I m4 && \
autoheader && \
libtoolize --copy --force --no-warn && \
automake --foreign --add-missing --copy && \
autoconf
