include $(top_srcdir)/m4/ltrc.inc

common = \
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
	WireProxy.h \
	WireProxy.cpp \
	WireStub.h \
	WireStub.cpp \
	Xml.h \
	Xml.cpp

if WIN32

lib_LTLIBRARIES = OOCore.la

OOCore_la_CPPFLAGS =  -I$(top_srcdir)/include
OOCore_la_CFLAGS = $(PTHREAD_CFLAGS)
OOCore_la_CXXFLAGS = $(PTHREAD_CFLAGS)
OOCore_la_LDFLAGS = $(PTHREAD_LIBS) -L../OOBase -loobase -module -avoid-version -no-undefined -Xlinker -lrpcrt4

OOCore_la_SOURCES = \
	$(common) \
	OOCore_res.rc

else

lib_LTLIBRARIES = liboocore.la

liboocore_la_CPPFLAGS =  -I$(top_srcdir)/include
liboocore_la_CFLAGS = $(PTHREAD_CFLAGS)
liboocore_la_CXXFLAGS = $(PTHREAD_CFLAGS)
liboocore_la_LDFLAGS = $(PTHREAD_LIBS) $(UUID_LIBS) $(LIBLTDL) -L../OOBase -loobase -release $(VERSION) -no-undefined

liboocore_la_SOURCES = \
	$(common)

endif
