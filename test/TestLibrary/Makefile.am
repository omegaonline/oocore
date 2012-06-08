include $(top_srcdir)/m4/ltrc.inc

check_LTLIBRARIES = testlibrary.la

testlibrary_la_CFLAGS = $(PTHREAD_CFLAGS)
testlibrary_la_CXXFLAGS = $(PTHREAD_CFLAGS)
testlibrary_la_LDFLAGS =  -no-undefined -module $(PTHREAD_LIBS) -rpath /nowhere

if WIN32
if WIN64_HYBRID
testlibrary_la_LIBADD = $(top_builddir)/src/OOCore/OOCore64.la
else
testlibrary_la_LIBADD = $(top_builddir)/src/OOCore/OOCore32.la
endif
testlibrary_la_LDFLAGS += -avoid-version
else
testlibrary_la_LIBADD = $(top_builddir)/src/OOCore/liboocore.la
testlibrary_la_LDFLAGS += -release $(VERSION) -no-undefined -export-symbols-regex '^(OID_|Omega_)'
endif

testlibrary_la_SOURCES = \
	../SimpleTest.h \
	TestLibrary.cpp
