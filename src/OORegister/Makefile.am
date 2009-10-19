include $(top_srcdir)/m4/ltrc.inc

bin_PROGRAMS = ooregister

ooregister_CPPFLAGS = -I$(top_srcdir)/include
ooregister_CFLAGS = $(PTHREAD_CFLAGS)
ooregister_CXXFLAGS = $(PTHREAD_CFLAGS)
ooregister_LDADD = ../OOBase/liboosvrbase.la ../OOBase/liboobase.la

if WIN32
ooregister_LDADD += ../OOCore/OOCore.la
else
ooregister_LDADD += ../OOCore/liboocore.la
ooregister_LDFLAGS = $(LIBLTDL)
endif

ooregister_SOURCES = \
	OORegister.cpp

if WIN32
ooregister_SOURCES += OORegister_res.rc
endif
