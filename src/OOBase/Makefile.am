noinst_LTLIBRARIES = liboobase.la liboosvrbase.la

liboobase_la_CPPFLAGS = -I@top_srcdir@/include
liboobase_la_CFLAGS = $(PTHREAD_CFLAGS) $(LTDLINCL)
liboobase_la_CXXFLAGS = $(PTHREAD_CFLAGS) $(LTDLINCL)
liboobase_la_LDFLAGS = $(PTHREAD_LIBS)

liboobase_la_SOURCES = \
	Atomic.h \
	Buffer.cpp \
	Buffer.h \
	ByteSwap.h \
	CDRStream.h \
	Condition.cpp \
	Condition.h \
	DLL.cpp \
	DLL.h \
	Destructor.h \
	Error.cpp \
	Mutex.cpp \
	Mutex.h \
	Once.cpp \
	Once.h \
	Queue.h \
	Singleton.h \
	SmartPtr.h \
	Socket.h \
	TLS.cpp \
	TLSSingleton.h \
	Thread.cpp \
	Thread.h \
	TimeVal.cpp \
	TimeVal.h \
	PosixSocket.h \
	PosixSocket.cpp \
	Win32.cpp \
	Win32.h \
	Win32Socket.cpp \
	Win32Socket.h \
	config-base.h \
	config-msvc.h \
	tr24731.h \
	tr24731.cpp \
	utf8.cpp \
	utf8.h

liboosvrbase_la_CPPFLAGS = -I@top_srcdir@/include
liboosvrbase_la_CFLAGS = $(PTHREAD_CFLAGS)
liboosvrbase_la_CXXFLAGS = $(PTHREAD_CFLAGS)
liboosvrbase_la_LDFLAGS = $(PTHREAD_LIBS)

if WIN32
liboosvrbase_la_LDFLAGS += -lnetapi32 -luserenv -lshlwapi
endif

liboosvrbase_la_SOURCES = \
	CmdArgs.cpp \
	CmdArgs.h \
	Logger.cpp \
	Logger.h \
	PipeWin32.cpp \
	Proactor.cpp \
	Proactor.h \
	ProactorEv.cpp \
	ProactorEv.h \
	ProactorWin32.cpp \
	ProactorWin32.h \
	pw_info.h \
	pw_info.cpp \
	SecurityWin32.cpp \
	SecurityWin32.h
