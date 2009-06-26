include $(top_srcdir)/m4/ltrc.inc

bin_PROGRAMS = ooregister

ooregister_CPPFLAGS = -I$(top_srcdir)/include
ooregister_CFLAGS = $(PTHREAD_CFLAGS)
ooregister_CXXFLAGS = $(PTHREAD_CFLAGS)
ooregister_LDFLAGS = -L../OOBase -loosvrbase -loobase $(PTHREAD_LIBS)

if WIN32
ooregister_LDADD = ../OOCore/OOCore.la
else
ooregister_LDADD = ../OOCore/liboocore.la
endif

ooregister_SOURCES = \
	OORegister.cpp
