#! /bin/sh
echo Running autotools...
aclocal -I m4 && \
autoheader && \
libtoolize --force --no-warn && \
automake --foreign --add-missing && \
autoconf
