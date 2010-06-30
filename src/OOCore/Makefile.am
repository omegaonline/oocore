include @top_srcdir@/m4/ltrc.inc

AM_CPPFLAGS =  -I@top_srcdir@/include -I@top_srcdir@/OOBase/include -I@top_builddir@/OOBase/include
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS)

common = \
	OOCore_precomp.h \
	md5.c \
	Activation.h \
	Activation.cpp \
	Compartment.h \
	Compartment.cpp \
	CDRMessage.h \
	Channel.h \
	Channel.cpp \
	Exception.h \
	Exception.cpp \
	Formatting.h \
	Formatting.cpp \
	IPS.h \
	LoopChannel.h \
	LoopChannel.cpp \
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
	WireStub.cpp

if WIN32

lib_LTLIBRARIES = OOCore.la

OOCore_la_LIBADD = @top_builddir@/OOBase/liboobase.la
OOCore_la_LDFLAGS = -module -avoid-version -no-undefined -lrpcrt4

OOCore_la_SOURCES = \
	$(common) \
	OOCore_res.rc

else

lib_LTLIBRARIES = liboocore.la

liboocore_la_LIBADD = @top_builddir@/OOBase/liboobase.la
liboocore_la_LDFLAGS = $(UUID_LIBS) $(LIBLTDL) -release $(VERSION) -no-undefined

liboocore_la_SOURCES = \
	$(common)

endif
