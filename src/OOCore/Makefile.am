include $(top_srcdir)/m4/ltrc.inc

lib_LTLIBRARIES = liboocore.la

liboocore_la_CPPFLAGS =  -I$(top_srcdir)/include
liboocore_la_CFLAGS = $(PTHREAD_CFLAGS)
liboocore_la_CXXFLAGS = $(PTHREAD_CFLAGS)
liboocore_la_LDFLAGS = $(PTHREAD_LIBS) -L../OOBase -loobase

if WIN32
liboocore_la_LDFLAGS += -avoid-version -no-undefined -Wl,'-lrpcrt4'
else
liboocore_la_LDFLAGS += -release $(VERSION)
endif

liboocore_la_SOURCES = \
	OOCore_precomp.h \
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
	IPS.h \
	OOCore.cpp \
	Registry.cpp \
	RunningObjectTable.cpp \
	StdObjectManager.h \
	StdObjectManager.cpp \
	Threading.cpp \
	TypeInfo.cpp \
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
	Xml.cpp \
	../Common/Server.h \
	../Common/Version.h

if WIN32
liboocore_la_SOURCES += OOCore_res.rc
endif
