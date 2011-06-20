include $(top_srcdir)/m4/ltrc.inc

AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS)

bin_PROGRAMS = oouuidgen

if WIN32
oouuidgen_LDADD = ../../src/OOCore/OOCore.la
else
oouuidgen_LDADD = ../../src/OOCore/liboocore.la
endif

oouuidgen_SOURCES = \
	main.cpp

if WIN32
oouuidgen_SOURCES += oouuidgen.rc
endif
