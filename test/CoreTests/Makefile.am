include $(top_srcdir)/m4/ltrc.inc

SUBDIRS = . TestLibrary TestProcess

# Change this to check_PROGRAMS = coretests
# when everything is a bit more stable!

noinst_PROGRAMS = coretests

coretests_CPPFLAGS = -I$(top_srcdir)/include
coretests_CFLAGS = $(PTHREAD_CFLAGS)
coretests_CXXFLAGS = $(PTHREAD_CFLAGS)
coretests_LDFLAGS = $(PTHREAD_LIBS)

if WIN32
coretests_LDADD = ../../src/OOCore/OOCore.la
else
coretests_LDADD = ../../src/OOCore/liboocore.la
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
	SimpleTest.h \
	UTF-8-test.txt
