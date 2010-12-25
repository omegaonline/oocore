include $(top_srcdir)/m4/ltrc.inc

AM_CPPFLAGS = -I@top_srcdir@/oobase/include -I@top_builddir@/oobase/include
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS)

common = \
	md5.c \
	Activation.cpp \
	Activation.h \
	CDRMessage.h \
	Channel.cpp \
	Channel.h \
	Compartment.cpp \
	Compartment.h \
	Exception.cpp \
	Exception.h \
	Formatting.cpp \
	Formatting.h \
	IO.cpp \
	IPS.h \
	LoopChannel.cpp \
	LoopChannel.h \
	OOCore.cpp \
	OOCore_precomp.h \
	Registry.cpp \
	RunningObjectTable.cpp \
	Server.h \
	StdObjectManager.cpp \
	StdObjectManager.h \
	Threading.cpp \
	TypeInfo.cpp \
	Types.cpp \
	UserSession.cpp \
	UserSession.h \
	WireProxy.cpp \
	WireProxy.h \
	WireStub.cpp \
	WireStub.h

if WIN32

lib_LTLIBRARIES = OOCore.la

OOCore_la_LIBADD = @top_builddir@/oobase/liboobase.la -lrpcrt4 -lshlwapi
OOCore_la_LDFLAGS = -module -avoid-version -no-undefined

OOCore_la_SOURCES = \
	$(common) \
	OOCore_res.rc

else

lib_LTLIBRARIES = liboocore.la

liboocore_la_LIBADD = @top_builddir@/oobase/liboobase.la $(UUID_LIBS) 
liboocore_la_LDFLAGS = -release $(VERSION) -no-undefined

liboocore_la_SOURCES = \
	$(common)

endif
