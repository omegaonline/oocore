include $(top_srcdir)/m4/ltrc.inc

noinst_LTLIBRARIES = liboosvrcmn.la liboosvrdb.la

liboosvrcmn_la_CPPFLAGS = -I@top_srcdir@/include
liboosvrcmn_la_CFLAGS = $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS)
liboosvrcmn_la_CXXFLAGS = $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS)
liboosvrcmn_la_LDFLAGS = $(PTHREAD_LIBS)

liboosvrcmn_la_SOURCES = \
	DebuggerWin32.cpp \
	MessageConnection.h \
	MessageConnection.cpp \
	Protocol.h

liboosvrdb_la_CPPFLAGS = -I@top_srcdir@/include
liboosvrdb_la_CFLAGS = $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS)
liboosvrdb_la_CXXFLAGS = $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS)
liboosvrdb_la_LDFLAGS = $(PTHREAD_LIBS)

liboosvrdb_la_SOURCES = \
	RegistryHive.h \
	RegistryHive.cpp

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
	MirrorKey.inl \
	RegistryCmn.h \
	IPSLite.h \
	OOSvrLite.cpp \
	RegistryLite.cpp

if WIN32
oosvrlite_la_SOURCES += OOSvrLite_res.rc
endif

bin_PROGRAMS = ooserverd

ooserverd_CPPFLAGS = -I$(top_srcdir)/include -DLIBEXEC_DIR=\"@libexecdir@\"
ooserverd_CFLAGS = $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS)
ooserverd_CXXFLAGS = $(PTHREAD_CFLAGS) $(SQLITE3_CFLAGS)
ooserverd_LDFLAGS =  $(SQLITE3_LIBS) $(EV_LIBS) $(PTHREAD_LIBS)
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

libexec_PROGRAMS = oosvruser

oosvruser_CPPFLAGS = -I$(top_srcdir)/include
oosvruser_CFLAGS = $(PTHREAD_CFLAGS)
oosvruser_CXXFLAGS = $(PTHREAD_CFLAGS)
oosvruser_LDFLAGS =  $(EV_LIBS) $(PTHREAD_LIBS)
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
	UserROT.cpp

if WIN32
oosvruser_SOURCES += OOSvrUser.rc
endif

EXTRA_DIST = \
	logo.ico \
	OOServer_msgs.bin

TESTS = \
	$(srcdir)/test_ooserverd.sh \
	$(srcdir)/test_oosvruser.sh

if !WIN32
bin_PROGRAMS += oo-launch

oo_launch_CPPFLAGS = -I$(top_srcdir)/include -DLIBEXEC_DIR=\"@libexecdir@\"
oo_launch_CFLAGS = $(PTHREAD_CFLAGS)
oo_launch_CXXFLAGS = $(PTHREAD_CFLAGS)
oo_launch_LDFLAGS = $(PTHREAD_LIBS)
oo_launch_LDADD = ../OOBase/liboosvrbase.la ../OOBase/liboobase.la

oo_launch_SOURCES = \
	Launch.cpp

endif
