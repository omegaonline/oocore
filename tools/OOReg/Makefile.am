include $(top_srcdir)/m4/ltrc.inc

bin_PROGRAMS = ooreg

ooreg_CPPFLAGS = -I$(top_srcdir)/include
ooreg_CFLAGS = $(PTHREAD_CFLAGS)
ooreg_CXXFLAGS = $(PTHREAD_CFLAGS)
ooreg_LDFLAGS =  $(PTHREAD_LIBS)
ooreg_LDADD = ../../src/OOBase/liboosvrbase.la ../../src/OOBase/liboobase.la

if WIN32
ooreg_LDADD += ../../src/OOCore/OOCore.la
else
ooreg_LDADD += ../../src/OOCore/liboocore.la
endif

ooreg_SOURCES = \
	commands.cpp \
	exception.cpp \
	main.cpp
