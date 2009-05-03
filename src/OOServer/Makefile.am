include $(top_srcdir)/build-aux/ltrc.inc

noinst_LIBRARIES = liboosvrcmn.a

liboosvrcmn_a_CPPFLAGS = -I@top_srcdir@/include $(SQLITE3_CPPFLAGS)
liboosvrcmn_a_CFLAGS = $(PTHREAD_CFLAGS)
liboosvrcmn_a_CXXFLAGS = $(PTHREAD_CFLAGS)

liboosvrcmn_a_SOURCES = \
	Database.h \
	Database.cpp \
	DebuggerWin32.cpp \
	MessageConnection.h \
	MessageConnection.cpp \
	Protocol.h \
	SQLite3_comp.c
		
bin_PROGRAMS = ooserverd oosvruser

ooserverd_CPPFLAGS = -I$(top_srcdir)/include $(SQLITE3_CPPFLAGS)
ooserverd_CFLAGS = $(PTHREAD_CFLAGS)
ooserverd_CXXFLAGS = $(PTHREAD_CFLAGS)
ooserverd_LDADD = ../OOCore/libOOCore.la
ooserverd_LDFLAGS = $(PTHREAD_LIBS) $(SQLITE3_LDFLAGS) -L. -loosvrcmn -L../OOBase -loobase -loosvrbase 

if WIN32
ooserverd_LDFLAGS += -lnetapi32 -luserenv -lshlwapi
endif

ooserverd_SOURCES = \
	OOServer_Root.h \
	ClientAcceptor.h \
	ClientAcceptor.cpp \
	RegistryHive.h \
	RegistryHive.cpp \
	RootMain.cpp \
	RootManager.h \
	RootManager.cpp \
	RootRegistry.cpp \
	RootWin32.cpp \
	SpawnedProcess.h \
	SpawnedProcessUnix.cpp \
	SpawnedProcessWin32.cpp
	
if WIN32
ooserverd_SOURCES += OOServer.rc
endif
		
oosvruser_CPPFLAGS = -I$(top_srcdir)/include $(SQLITE3_CPPFLAGS)
oosvruser_CFLAGS = $(PTHREAD_CFLAGS)
oosvruser_CXXFLAGS = $(PTHREAD_CFLAGS)
oosvruser_LDADD = ../OOCore/libOOCore.la
oosvruser_LDFLAGS = $(PTHREAD_LIBS) $(SQLITE3_LDFLAGS) -L. -loosvrcmn -L../OOBase -loobase -loosvrbase 

if WIN32
oosvruser_LDFLAGS += -lshlwapi -lnetapi32 -luserenv
endif

oosvruser_SOURCES = \
	Channel.h \
	Channel.cpp \
	InterProcessService.h \
	InterProcessService.cpp \
	OOServer_User.h \
	UserAcceptor.h \
	UserAcceptor.cpp \
	UserMain.cpp \
	UserManager.h \
	UserManager.cpp \
	UserNet.h \
	UserNet.cpp \
	UserProcessWin32.cpp \
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
