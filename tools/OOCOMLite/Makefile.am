#AM_CPPFLAGS =
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS)

if WIN32

oolibexecdir = @libexecdir@/omegaonline
oolibexec_LTLIBRARIES = OOCOMLite.la

OOCOMLite_la_LDFLAGS = -module -no-undefined -avoid-version -Wl,-luuid -Wl,--kill-at
OOCOMLite_la_LIBADD = $(top_builddir)/src/OOCore/OOCore.la -loleaut32

OOCOMLite_la_SOURCES = \
	classfactory.cpp \
	dispatch.h \
	disp_nspace.h \
	disp_nspace.cpp \
	disp_obj.h \
	disp_obj.cpp \
	main.cpp

endif
