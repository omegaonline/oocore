AM_CPPFLAGS = -I@top_srcdir@/oobase/include -I@top_builddir@/oobase/include
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDFLAGS = $(PTHREAD_LIBS)

if !WIN32
bin_PROGRAMS = oo-launch

oo_launch_CPPFLAGS = $(AM_CPPFLAGS) -DLIBEXEC_DIR=\"@libexecdir@/omegaonline\"
oo_launch_LDADD = @top_builddir@/oobase/liboosvrbase.la @top_builddir@/oobase/liboobase.la

oo_launch_SOURCES = \
	Launch.cpp

endif
