include $(top_srcdir)/m4/ltrc.inc

check_LTLIBRARIES = testlibrary.la

testlibrary_la_CPPFLAGS = -I$(top_srcdir)/include
testlibrary_la_CFLAGS = $(PTHREAD_CFLAGS)
testlibrary_la_CXXFLAGS = $(PTHREAD_CFLAGS)
testlibrary_la_LDFLAGS = -module $(PTHREAD_LIBS) -rpath /nowhere

if WIN32
testlibrary_la_LIBADD = $(top_builddir)/src/OOCore/OOCore.la
testlibrary_la_LDFLAGS += -avoid-version -no-undefined
else
testlibrary_la_LIBADD = $(top_builddir)/OOCore/liboocore.la
testlibrary_la_LDFLAGS += -release $(VERSION) -no-undefined
endif

testlibrary_la_SOURCES = \
	SinpleTest.h \
	TestLibrary.cpp
