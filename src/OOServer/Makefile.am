include $(top_srcdir)/m4/ltrc.inc

AM_CPPFLAGS = -I@top_srcdir@/oobase/include -I@top_builddir@/oobase/include
AM_CFLAGS = $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS) $(SQLITE3_LIBS)

noinst_LTLIBRARIES = liboosvrcmn.la liboosvrdb.la

liboosvrcmn_la_SOURCES = \
	DebuggerWin32.cpp \
	MessageConnection.h \
	MessageConnection.cpp \
	Protocol.h

liboosvrdb_la_SOURCES = \
	RegistryHive.h \
	RegistryHive.cpp

oolibexecdir = @libexecdir@/omegaonline
oolibexec_LTLIBRARIES = oosvrlite.la

oosvrlite_la_LDFLAGS = -module -no-undefined
oosvrlite_la_LIBADD = liboosvrdb.la @top_builddir@/oobase/liboosvrbase.la @top_builddir@/oobase/liboobase.la

if WIN32
oosvrlite_la_LIBADD += ../OOCore/OOCore.la
oosvrlite_la_LDFLAGS += -avoid-version
else
oosvrlite_la_LIBADD += ../OOCore/liboocore.la
oosvrlite_la_LDFLAGS += -release $(VERSION)
endif

oosvrlite_la_SOURCES = \
	OOServer_Lite.h \
	MirrorKey.inl \
	RegistryCmn.h \
	IPSLite.h \
	OOSvrLite.cpp \
	RegistryLite.cpp

if WIN32
oosvrlite_la_SOURCES += OOSvrLite_res.rc
endif

bin_PROGRAMS = ooserverd

ooserverd_CPPFLAGS = $(AM_CPPFLAGS) -DLIBEXEC_DIR=\"@libexecdir@/omegaonline\"
ooserverd_LDADD = liboosvrcmn.la liboosvrdb.la @top_builddir@/oobase/liboosvrbase.la @top_builddir@/oobase/liboobase.la

ooserverd_SOURCES = \
	OOServer_Root.h \
	ClientAcceptor.h \
	ClientAcceptor.cpp \
	RootMain.cpp \
	RootManager.h \
	RootManager.cpp \
	RootRegistry.cpp \
	RootServices.cpp \
	RootUnix.cpp \
	RootWin32.cpp \
	SpawnedProcess.h \
	SpawnedProcessUnix.cpp \
	SpawnedProcessWin32.cpp

if WIN32
ooserverd_SOURCES += OOServer.rc
endif

libexec_PROGRAMS = oosvruser

oosvruser_LDADD = liboosvrcmn.la @top_builddir@/oobase/liboosvrbase.la @top_builddir@/oobase/liboobase.la

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
	MirrorKey.inl \
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
	UserROT.cpp \
	UserServices.cpp

if WIN32
oosvruser_SOURCES += OOSvrUser.rc
endif

EXTRA_DIST = \
	logo.ico \
	OOServer_msgs.bin
