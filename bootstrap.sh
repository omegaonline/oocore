#! /bin/sh
echo Running autotools...

if test ! -d "./oobase"; then
	echo You need a link to OOBase
fi

aclocal -I m4 && \
autoheader && \
libtoolize --copy --force --no-warn && \
automake --foreign --add-missing --copy && \
autoconf

echo Bootstrapping OOBase...
cd oobase
./bootstrap.sh
cd ..
