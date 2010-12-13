include $(top_srcdir)/m4/ltrc.inc

AM_CPPFLAGS = -I@top_srcdir@/oobase/include -I@top_builddir@/oobase/include
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS)

bin_PROGRAMS = ooreg

ooreg_LDADD = @top_builddir@/oobase/liboosvrbase.la @top_builddir@/oobase/liboobase.la

if WIN32
ooreg_LDADD += ../../src/OOCore/OOCore.la
else
ooreg_LDADD += ../../src/OOCore/liboocore.la
endif

ooreg_SOURCES = \
	commands.cpp \
	exception.cpp \
	main.cpp

if WIN32
ooreg_SOURCES += OOReg.rc
endif
