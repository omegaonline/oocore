include $(top_srcdir)/m4/ltrc.inc

SUBDIRS = TestLibrary TestProcess TestService .

CLEANFILES = *.regdb

check_PROGRAMS = coretests

coretests_CPPFLAGS = -DBUILD_DIR=$(abs_builddir) -DTOP_SRC_DIR=$(abs_top_srcdir)
coretests_CFLAGS = $(PTHREAD_CFLAGS)
coretests_CXXFLAGS = $(PTHREAD_CFLAGS)
coretests_LDFLAGS = $(PTHREAD_LIBS)

if WIN32
if WIN64_HYBRID
coretests_LDADD = $(top_builddir)/src/OOCore/OOCore64.la
else
coretests_LDADD = $(top_builddir)/src/OOCore/OOCore32.la
endif
else
coretests_LDADD = $(top_builddir)/src/OOCore/liboocore.la
endif

coretests_SOURCES = \
	any.cpp \
	compartment.cpp \
	core.cpp \
	interfaces.cpp \
	interfaces.h \
	main.cpp \
	net.cpp \
	registry.cpp \
	strings.cpp \
	services.cpp \
	Test.h

EXTRA_DIST = \
	test.conf \
	coretests.sh \
	UTF-16-test.txt \
	UTF-8-test.txt

if DEBUG
TESTS_ENVIRONMENT = OMEGA_DEBUG=true
endif

TESTS = \
	$(srcdir)/coretests.sh
