include $(top_srcdir)/m4/ltrc.inc

AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS)

bin_PROGRAMS = ooguidgen

if WIN32
ooguidgen_LDADD = ../../src/OOCore/OOCore.la
else
ooguidgen_LDADD = ../../src/OOCore/liboocore.la
endif

ooguidgen_SOURCES = \
	main.cpp

if WIN32
ooguidgen_SOURCES += ooguidgen.rc
endif
