include $(top_srcdir)/m4/ltrc.inc

check_PROGRAMS = testprocess

testprocess_CFLAGS = $(PTHREAD_CFLAGS)
testprocess_CXXFLAGS = $(PTHREAD_CFLAGS)
testprocess_LDFLAGS = $(PTHREAD_LIBS)

if WIN32
if WIN64_HYBRID
testprocess_LDADD = $(top_builddir)/src/OOCore/OOCore64.la
else
testprocess_LDADD = $(top_builddir)/src/OOCore/OOCore32.la
endif
else
testprocess_LDADD = $(top_builddir)/src/OOCore/liboocore.la
endif

testprocess_SOURCES = \
	../SimpleTest.h \
	TestProcess.cpp

