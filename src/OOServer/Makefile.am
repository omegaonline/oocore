include $(top_srcdir)/build-aux/ltrc.inc

noinst_LIBRARIES = liboosvrcmn.a

liboosvrcmn_a_CPPFLAGS = -I@top_srcdir@/include $(ACE_CPPFLAGS) $(SQLITE3_CPPFLAGS)
liboosvrcmn_a_CFLAGS = $(PTHREAD_CFLAGS)
liboosvrcmn_a_CXXFLAGS = $(PTHREAD_CFLAGS)

liboosvrcmn_a_SOURCES = \
	Database.h \
	Database.cpp \
	DebuggerWin32.cpp \
	MessageConnection.h \
	MessageConnection.cpp \
	MessagePipe.h \
	MessagePipe.inl \
	MessagePipeWin32.cpp \
	MessagePipeUnix.cpp \
	Protocol.h \
	SQLite3_comp.c
		
bin_PROGRAMS = ooserverd oosvruser

ooserverd_CPPFLAGS = -I$(top_srcdir)/include $(ACE_CPPFLAGS) $(SQLITE3_CPPFLAGS)
ooserverd_CFLAGS = $(PTHREAD_CFLAGS)
ooserverd_CXXFLAGS = $(PTHREAD_CFLAGS)
ooserverd_LDADD = liboosvrcmn.a ../OOCore/libOOCore.la
ooserverd_LDFLAGS = $(PTHREAD_LIBS) $(ACE_LDFLAGS) $(SQLITE3_LDFLAGS)

if WIN32
ooserverd_LDFLAGS += -lmswsock -lnetapi32 -luserenv -lshlwapi
endif

ooserverd_SOURCES = \
	NTService.h \
	NTService.cpp \
	OOServer_Root.h \
	RegistryHive.h \
	RegistryHive.cpp \
	RootHttp.h \
	RootHttp.cpp \
	RootMain.cpp \
	RootManager.h \
	RootManager.cpp \
	RootRegistry.cpp \
	SpawnedProcess.h \
	SpawnedProcessUnix.cpp \
	SpawnedProcessWin32.cpp
	
if WIN32
ooserverd_SOURCES += OOServer.rc
endif
		
oosvruser_CPPFLAGS = -I$(top_srcdir)/include $(ACE_CPPFLAGS) $(SQLITE3_CPPFLAGS)
oosvruser_CFLAGS = $(PTHREAD_CFLAGS)
oosvruser_CXXFLAGS = $(PTHREAD_CFLAGS)
oosvruser_LDADD = liboosvrcmn.a ../OOCore/libOOCore.la
oosvruser_LDFLAGS = $(PTHREAD_LIBS) $(ACE_LDFLAGS) $(SQLITE3_LDFLAGS)

if WIN32
oosvruser_LDFLAGS += -lshlwapi
endif

oosvruser_SOURCES = \
	Channel.h \
	Channel.cpp \
	InterProcessService.h \
	InterProcessService.cpp \
	NetHttp.h \
	NetHttp.cpp \
	NetTcp.h \
	NetTcp.cpp \
	OOServer_User.h \
	UserHttp.h \
	UserHttp.cpp \
	UserMain.cpp \
	UserManager.h \
	UserManager.cpp \
	UserNet.h \
	UserNet.cpp \
	UserRegistry.h \
	UserRegistry.cpp \
	UserROT.h \
	UserROT.cpp

if WIN32
oosvruser_SOURCES += OOSvrUser.rc
endif

EXTRA_DIST = \
	logo.ico \
	NT_msgs.bin
