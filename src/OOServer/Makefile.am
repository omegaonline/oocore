include $(top_srcdir)/m4/ltrc.inc

noinst_LTLIBRARIES = liboosvrcmn.la liboosvrdb.la

liboosvrcmn_la_CPPFLAGS = -I@top_srcdir@/include
liboosvrcmn_la_CFLAGS = $(PTHREAD_CFLAGS)
liboosvrcmn_la_CXXFLAGS = $(PTHREAD_CFLAGS)

liboosvrcmn_la_SOURCES = \
	DebuggerWin32.cpp \
	MessageConnection.h \
	MessageConnection.cpp \
	Protocol.h

liboosvrdb_la_CPPFLAGS = -I@top_srcdir@/include
liboosvrdb_la_CFLAGS = $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS)
liboosvrdb_la_CXXFLAGS = $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS)

liboosvrdb_la_SOURCES = \
	Database.h \
	Database.cpp \
	RegistryHive.h \
	RegistryHive.cpp \
	SQLite3_comp.c

oolibexecdir = $(libexecdir)/omegaonline
oolibexec_LTLIBRARIES = oosvrlite.la

oosvrlite_la_CPPFLAGS =  -I$(top_srcdir)/include
oosvrlite_la_CFLAGS = $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS)
oosvrlite_la_CXXFLAGS = $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS)
oosvrlite_la_LDFLAGS = $(SQLITE3_LIBS) $(PTHREAD_LIBS) -module -no-undefined
oosvrlite_la_LIBADD = liboosvrdb.la ../OOBase/liboosvrbase.la ../OOBase/liboobase.la

if WIN32
oosvrlite_la_LIBADD += ../OOCore/OOCore.la
oosvrlite_la_LDFLAGS += -avoid-version
else
oosvrlite_la_LIBADD += ../OOCore/liboocore.la
oosvrlite_la_LDFLAGS += -release $(VERSION)
endif

oosvrlite_la_SOURCES = \
	OOServer_Lite.h \
	RegistryCmn.h \
	IPSLite.h \
	IPSLite.cpp \
	OOSvrLite.cpp

if WIN32
oosvrlite_la_SOURCES += OOSvrLite_res.rc
endif

bin_PROGRAMS = ooserverd oosvruser

ooserverd_CPPFLAGS = -I$(top_srcdir)/include
ooserverd_CFLAGS = $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS)
ooserverd_CXXFLAGS = $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS)
ooserverd_LDFLAGS = $(SQLITE3_LIBS) $(PTHREAD_LIBS)
ooserverd_LDADD = liboosvrcmn.la liboosvrdb.la ../OOBase/liboosvrbase.la ../OOBase/liboobase.la

ooserverd_SOURCES = \
	OOServer_Root.h \
	ClientAcceptor.h \
	ClientAcceptor.cpp \
	RootMain.cpp \
	RootManager.h \
	RootManager.cpp \
	RootRegistry.cpp \
	RootUnix.cpp \
	RootWin32.cpp \
	SpawnedProcess.h \
	SpawnedProcessUnix.cpp \
	SpawnedProcessWin32.cpp

if WIN32
ooserverd_SOURCES += OOServer.rc
endif

oosvruser_CPPFLAGS = -I$(top_srcdir)/include
oosvruser_CFLAGS = $(PTHREAD_CFLAGS)
oosvruser_CXXFLAGS = $(PTHREAD_CFLAGS)
oosvruser_LDFLAGS = $(PTHREAD_LIBS)
oosvruser_LDADD = liboosvrcmn.la ../OOBase/liboosvrbase.la ../OOBase/liboobase.la

if WIN32
oosvruser_LDADD += ../OOCore/OOCore.la
else
oosvruser_LDADD += ../OOCore/liboocore.la
endif

oosvruser_SOURCES = \
	OOServer_User.h \
	Channel.h \
	Channel.cpp \
	InterProcessService.h \
	InterProcessService.cpp \
	RegistryCmn.h \
	UserAcceptor.h \
	UserAcceptor.cpp \
	UserMain.cpp \
	UserManager.h \
	UserManager.cpp \
	UserNet.h \
	UserNet.cpp \
	UserProcess.h \
	UserProcessWin32.cpp \
	UserProcessUnix.cpp \
	UserRegistry.h \
	UserRegistry.cpp \
	UserROT.h \
	UserROT.cpp

if WIN32
oosvruser_SOURCES += OOSvrUser.rc
endif

EXTRA_DIST = \
	logo.ico \
	OOServer_msgs.bin
