include $(top_srcdir)/m4/ltrc.inc

AM_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/OOBase/include -I@top_builddir@/OOBase/include
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS)

bin_PROGRAMS = ooreg

ooreg_LDADD = @top_builddir@/OOBase/liboosvrbase.la @top_builddir@/OOBase/liboobase.la

if WIN32
ooreg_LDADD += ../../src/OOCore/OOCore.la
else
ooreg_LDADD += ../../src/OOCore/liboocore.la
endif

ooreg_SOURCES = \
	commands.cpp \
	exception.cpp \
	main.cpp
