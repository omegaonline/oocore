include $(top_srcdir)/m4/ltrc.inc

AM_CPPFLAGS = -I@top_srcdir@/oobase/include -I@top_builddir@/oobase/include
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS)

if WIN32
AM_CPPFLAGS += -D_WIN32_WINNT=0x0502
endif

bin_PROGRAMS = ooreg

ooreg_LDADD = @top_builddir@/oobase/liboobase.la

if WIN32
if WIN64_HYBRID
ooreg_LDADD += $(top_builddir)/src/OOCore/OOCore64.la
else
ooreg_LDADD += $(top_builddir)/src/OOCore/OOCore32.la
endif
else
ooreg_LDADD += $(top_builddir)/src/OOCore/liboocore.la
endif

ooreg_SOURCES = \
	main.cpp

if WIN32
ooreg_SOURCES += ooreg.rc
endif
