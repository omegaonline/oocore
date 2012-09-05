include $(top_srcdir)/m4/ltrc.inc

AM_CPPFLAGS = -I@top_srcdir@/oobase/include -I@top_builddir@/oobase/include $(SQLITE3_CFLAGS) 
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS)

if WIN32
AM_CPPFLAGS += -D_WIN32_WINNT=0x0502
endif

noinst_LTLIBRARIES = libdb.la

libdb_la_SOURCES = \
	Common.h \
	Database.h \
	Database.cpp \
	RegistryHive.h \
	RegistryHive.cpp \
	SQLite3_comp.c
	
libdb_la_LIBADD = $(SQLITE3_LIBS)
