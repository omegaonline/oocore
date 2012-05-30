include $(top_srcdir)/m4/ltrc.inc

AM_CPPFLAGS = -I@top_srcdir@/oobase/include -I@top_builddir@/oobase/include $(DBUS_CFLAGS)
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS)

if WIN32
AM_CPPFLAGS += -D_WIN32_WINNT=0x0502
endif

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
	Launch.cpp \
	LoopChannel.cpp \
	LoopChannel.h \
	OOCore.cpp \
	OOCore_precomp.h \
	RunningObjectTable.cpp \
	Safe.cpp \
	Server.h \
	SimpleMarshaller.cpp \
	StdObjectManager.cpp \
	StdObjectManager.h \
	Text.cpp \
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

liboocore_la_LIBADD = @top_builddir@/oobase/liboobase.la $(UUID_LIBS) $(DBUS_LIBS)
liboocore_la_LDFLAGS = -release $(VERSION) -no-undefined -export-symbols-regex '^(OID_|OOCore_)'

liboocore_la_SOURCES = \
	$(common)

endif
