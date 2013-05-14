include $(top_srcdir)/m4/ltrc.inc

SUBDIRS = libdb

AM_CPPFLAGS = -I@top_srcdir@/oobase/include -I@top_builddir@/oobase/include
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS)

if WIN32
AM_CPPFLAGS += -D_WIN32_WINNT=0x0502
endif

bin_PROGRAMS = ooserverd

ooserverd_CPPFLAGS = $(SQLITE3_CFLAGS) \
	-DLIBEXEC_DIR=\"$(pkglibexecdir)/\" \
	-DCONFIG_DIR=\"$(pkgdatadir)/\" \
	-DREGDB_PATH=\"@localstatedir@/lib/@PACKAGE@/\" \
	$(AM_CPPFLAGS) 
		
ooserverd_LDADD = libdb/libdb.la @top_builddir@/oobase/liboobase.la

ooserverd_SOURCES = \
	RootMain.cpp \
	RootManager.cpp \
	RootClientConn.cpp \
	RootRegistryConn.cpp \
	RootUserConn.cpp \
	RootUnix.cpp \
	RootWin32.cpp \
	RootRegistry.cpp \
	RootServices.cpp	

if WIN32
ooserverd_SOURCES += OOServer.rc
ooserverd_LDADD += -lole32 -lrpcrt4
endif

pkglibexec_PROGRAMS = oosvrreg

oosvrreg_SOURCES = \
	RegistryManager.cpp \
	RegistryMain.cpp \
	RegistryRootConn.cpp	

oosvrreg_CPPFLAGS = $(AM_CPPFLAGS)
oosvrreg_LDADD = @top_builddir@/oobase/liboobase.la

if WIN32
oosvrreg_SOURCES += OOServer.rc
endif

pkglibexec_PROGRAMS += oosvruser

oosvruser_SOURCES = \
	UserIPS.cpp \
	UserMain.cpp \
	UserManager.cpp \
	UserProcessWin32.cpp \
	UserProcessUnix.cpp \
	UserRegistry.cpp \
	UserRootConn.cpp \
	UserROT.cpp \
	UserServices.cpp

oosvruser_CPPFLAGS = $(AM_CPPFLAGS)
oosvruser_LDADD = @top_builddir@/oobase/liboobase.la

if WIN32
if WIN64_HYBRID

oosvruser_CPPFLAGS += -DWIN64_HYBRID $(AM_CPPFLAGS)
oosvruser_LDADD += ../OOCore/OOCore64.la

pkglibexec_PROGRAMS += oosvrhost64

oosvrhost64_SOURCES = \
	HostMain.cpp \
	HostService.cpp \
	HostSurrogate.cpp \
	OOSvrHost.rc

oosvrhost64_LDADD = @top_builddir@/oobase/liboobase.la ../OOCore/OOCore64.la -lole32

else

pkglibexec_PROGRAMS += oosvrhost32

oosvruser_LDADD += ../OOCore/OOCore32.la

oosvrhost32_SOURCES = \
	HostMain.cpp \
	HostService.cpp \
	HostSurrogate.cpp \
	OOSvrHost.rc

oosvrhost32_LDADD = @top_builddir@/oobase/liboobase.la ../OOCore/OOCore32.la -lole32

endif

oosvruser_SOURCES += OOSvrUser.rc

else

oosvruser_LDADD += @top_builddir@/oobase/liboobase.la ../OOCore/liboocore.la

pkglibexec_PROGRAMS += oosvrhost

oosvrhost_SOURCES = \
	HostMain.cpp \
	HostService.cpp \
	HostSurrogate.cpp

oosvrhost_LDADD = @top_builddir@/oobase/liboobase.la ../OOCore/liboocore.la

endif

EXTRA_DIST = \
	asInvoker_manifest.xml \
	logo.ico \
	OOServer_msgs.bin
