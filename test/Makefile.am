include $(top_srcdir)/m4/ltrc.inc

SUBDIRS = TestLibrary TestProcess .

CLEANFILES = *.regdb

check_PROGRAMS = coretests

coretests_CPPFLAGS = -DBUILD_DIR=$(abs_builddir) -DTOP_SRC_DIR=$(abs_top_srcdir)
coretests_CFLAGS = $(PTHREAD_CFLAGS)
coretests_CXXFLAGS = $(PTHREAD_CFLAGS)
coretests_LDFLAGS = $(PTHREAD_LIBS)

if WIN32
coretests_LDADD = ../src/OOCore/OOCore.la
else
coretests_LDADD = ../src/OOCore/liboocore.la
endif

coretests_SOURCES = \
	aggregator.h \
	any.cpp \
	compartment.cpp \
	core.cpp \
	interfaces.cpp \
	interfaces.h \
	main.cpp \
	net.cpp \
	registry.cpp \
	strings.cpp \
	Test.h

EXTRA_DIST = \
	test.conf \
	coretests.sh \
	UTF-16-test.txt \
	UTF-8-test.txt

if DEBUG
TESTS_ENVIRONMENT = \
	OMEGA_DEBUG=yes \
	OMEGA_USER_BINARY=$(abs_top_builddir)/src/OOServer/oosvruser 
endif

TESTS = \
	$(srcdir)/coretests.sh
