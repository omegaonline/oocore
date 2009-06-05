noinst_LIBRARIES = liboobase.a liboosvrbase.a

liboobase_a_CPPFLAGS = -I@top_srcdir@/include
liboobase_a_CFLAGS = $(PTHREAD_CFLAGS)
liboobase_a_CXXFLAGS = $(PTHREAD_CFLAGS)

liboobase_a_SOURCES = \
	Atomic.h \
	Buffer.cpp \
	Buffer.h \
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
	Swap.h \
	TLS.cpp \
	TLSSingleton.h \
	Thread.cpp \
	Thread.h \
	TimeVal.cpp \
	TimeVal.h \
	UnixSocket.cpp \
	Win32.cpp \
	Win32.h \
	Win32Socket.cpp \
	Win32Socket.h \
	config-base.h \
	config-msvc.h \
	config-win32.h \
	tr24731.h \
	tr24731.cpp \
	utf8.cpp \
	utf8.h

liboosvrbase_a_CPPFLAGS = -I@top_srcdir@/include
liboosvrbase_a_CFLAGS = $(PTHREAD_CFLAGS)
liboosvrbase_a_CXXFLAGS = $(PTHREAD_CFLAGS)

liboosvrbase_a_SOURCES = \
	CmdArgs.cpp \
	CmdArgs.h \
	Logger.cpp \
	Logger.h \
	PipeWin32.cpp \
	Proactor.cpp \
	Proactor.h \
	ProactorWin32.cpp \
	ProactorWin32.h \
	SecurityWin32.cpp \
	SecurityWin32.h
