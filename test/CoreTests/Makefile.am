include $(top_srcdir)/m4/ltrc.inc

SUBDIRS = . TestLibrary TestProcess

check_PROGRAMS = coretests

coretests_CPPFLAGS = -I$(top_srcdir)/include -DBUILD_DIR=$(abs_builddir) -DTOP_SRC_DIR=$(abs_top_srcdir)
coretests_CFLAGS = $(PTHREAD_CFLAGS)
coretests_CXXFLAGS = $(PTHREAD_CFLAGS)
coretests_LDFLAGS = $(PTHREAD_LIBS)

if WIN32
coretests_LDADD = $(top_builddir)/src/OOCore/OOCore.la
else
coretests_LDADD = $(top_builddir)/src/OOCore/liboocore.la
endif

coretests_SOURCES = \
	apartment.cpp \
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
