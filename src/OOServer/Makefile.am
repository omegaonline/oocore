include $(top_srcdir)/m4/ltrc.inc

AM_CPPFLAGS = -I@top_srcdir@/oobase/include -I@top_builddir@/oobase/include
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS)

if WIN32
AM_CPPFLAGS += -D_WIN32_WINNT=0x0502
endif

noinst_LTLIBRARIES = liboosvrcmn.la

liboosvrcmn_la_SOURCES = \
	DebuggerWin32.cpp \
	MessageConnection.h \
	MessageConnection.cpp \
	Protocol.h

bin_PROGRAMS = ooserverd

ooserverd_CPPFLAGS = $(AM_CPPFLAGS) $(SQLITE3_CFLAGS) \
	-DLIBEXEC_DIR=\"$(pkglibexecdir)/\" \
	-DCONFIG_DIR=\"$(pkgdatadir)/\" \
	-DREGDB_PATH=\"@localstatedir@/lib/@PACKAGE@/\" 
		
ooserverd_LDADD = liboosvrcmn.la @top_builddir@/libdb/libdb.la @top_builddir@/oobase/liboosvrbase.la @top_builddir@/oobase/liboobase.la

ooserverd_SOURCES = \
	OOServer_Root.h \
	RootMain.cpp \
	RootManager.h \
	RootManager.cpp \
	RootRegistry.cpp \
	RootServices.cpp \
	RootUnix.cpp \
	RootWin32.cpp \
	RootProcess.h \
	RootProcessUnix.cpp \
	RootProcessWin32.cpp

if WIN32
ooserverd_SOURCES += OOServer.rc
ooserverd_LDADD += -lshlwapi
endif

pkglibexec_PROGRAMS = oosvruser oosvrhost

oosvruser_LDADD = liboosvrcmn.la @top_builddir@/oobase/liboosvrbase.la @top_builddir@/oobase/liboobase.la
oosvrhost_LDADD = @top_builddir@/oobase/liboobase.la

if WIN32
oosvruser_LDADD += ../OOCore/OOCore.la
oosvrhost_LDADD += ../OOCore/OOCore.la
else
oosvruser_LDADD += ../OOCore/liboocore.la
oosvrhost_LDADD += ../OOCore/liboocore.la
endif

oosvruser_SOURCES = \
	OOServer_User.h \
	UserChannel.h \
	UserChannel.cpp \
	UserIPS.h \
	UserIPS.cpp \
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
	
oosvrhost_SOURCES = \
	OOServer_Host.h \
	HostMain.cpp \
	HostSurrogate.cpp

if WIN32
oosvruser_SOURCES += OOSvrUser.rc
oosvrhost_SOURCES += OOSvrHost.rc
endif

EXTRA_DIST = \
	asInvoker_manifest.xml \
	logo.ico \
	OOServer_msgs.bin
