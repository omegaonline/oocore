include $(top_srcdir)/m4/ltrc.inc

check_PROGRAMS = testprocess

testprocess_CPPFLAGS = -I$(top_srcdir)/include
testprocess_CFLAGS = $(PTHREAD_CFLAGS)
testprocess_CXXFLAGS = $(PTHREAD_CFLAGS)
testprocess_LDFLAGS = $(PTHREAD_LIBS)

if WIN32
testprocess_LDADD = $(top_builddir)/src/OOCore/OOCore.la
else
testprocess_LDADD = $(top_builddir)/src/OOCore/liboocore.la
endif

testprocess_SOURCES = \
	../SimpleTest.h \
	TestProcess.cpp

