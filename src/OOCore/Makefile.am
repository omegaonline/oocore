include $(top_srcdir)/build-aux/ltrc.inc

lib_LTLIBRARIES = libOOCore.la

libOOCore_la_CPPFLAGS =  -I$(top_srcdir)/include $(ACE_CPPFLAGS)
libOOCore_la_CFLAGS = $(PTHREAD_CFLAGS)
libOOCore_la_CXXFLAGS = $(PTHREAD_CFLAGS)
libOOCore_la_LDFLAGS = $(PTHREAD_LIBS) $(ACE_LDFLAGS)

if WIN32
libOOCore_la_LDFLAGS += -no-undefined -Wl,'-lrpcrt4'
endif

libOOCore_la_SOURCES = \
	md5.c \
	Activation.h \
	Activation.cpp \
	ApartmentImpl.h \
	ApartmentImpl.cpp \
	CDRMessage.h \
	Channel.h \
	Channel.cpp \
	Exception.h \
	Exception.cpp \
	HttpImpl.h \
	HttpImpl.cpp \
	OOCore_precomp.h \
	OOCore.cpp \
	Registry.cpp \
	RunningObjectTable.cpp \
	StdObjectManager.h \
	StdObjectManager.cpp \
	Threading.cpp \
	Types.cpp \
	UserSession.h \
	UserSession.cpp \
	WireImpl.h \
	WireImpl.cpp \
	WireProxy.h \
	WireProxy.cpp \
	WireStub.h \
	WireStub.cpp \
	Xml.h \
	Xml.cpp
	
if WIN32
libOOCore_la_SOURCES += OOCore_res.rc
endif
