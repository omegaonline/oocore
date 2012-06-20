include $(top_srcdir)/m4/ltrc.inc

check_LTLIBRARIES = testservice.la

testservice_la_CFLAGS = $(PTHREAD_CFLAGS)
testservice_la_CXXFLAGS = $(PTHREAD_CFLAGS)
testservice_la_LDFLAGS =  -no-undefined -module $(PTHREAD_LIBS) -rpath /nowhere

if WIN32
if WIN64_HYBRID
testservice_la_LIBADD = $(top_builddir)/src/OOCore/OOCore64.la
else
testservice_la_LIBADD = $(top_builddir)/src/OOCore/OOCore32.la
endif
testservice_la_LDFLAGS += -avoid-version
else
testservice_la_LIBADD = $(top_builddir)/src/OOCore/liboocore.la
testservice_la_LDFLAGS += -release $(VERSION) -no-undefined -export-symbols-regex '^(OID_|Omega_)'
endif

testservice_la_SOURCES = \
	TestService.cpp
