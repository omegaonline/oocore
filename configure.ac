AC_PREREQ(2.61)

AC_INIT(omegaonline,0.5.0)
AC_CONFIG_SRCDIR([src/OOCore/OOCore.cpp])
AC_CONFIG_HEADER([src/OOBase/config-autoconf.h])

# AC_CONFIG_HEADER([src/Common/autoheader_dummy.h src/Common/config.h])
# AX_PREFIX_CONFIG_H([src/Common/config-autoconf.h],[OMEGA])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([
	Makefile
	include/Makefile
	src/Makefile
	src/OOBase/Makefile
	src/OOCore/Makefile
	src/OOServer/Makefile
])

AM_INIT_AUTOMAKE([foreign 1.10])
AM_MAINTAINER_MODE

AC_CANONICAL_HOST

# Add the --enable-debug arg
AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[Turn on debugging]),[debug=true],[debug=false])
AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])

OO_PROG_CC

# Set sensible default CFLAGS if necessary
if test "$oo_test_CFLAGS" != set; then
      
  # Always try to hide symbols
  AX_CFLAGS_GCC_OPTION([-fvisiblity=hidden])
  
  if test x$debug = xtrue; then
    AX_CFLAGS_GCC_OPTION([-g])
  else
    AX_CFLAGS_GCC_OPTION([-O3])
  fi
fi

OO_PROG_CXX

# Set sensible default CXXFLAGS if necessary
if test "$oo_test_CXXFLAGS" != set; then
    
  # Always add full warnings
  AX_CXXFLAGS_WARN_ALL
  
  # Always try to hide symbols
  AX_CXXFLAGS_GCC_OPTION([-fvisiblity=hidden])
  
  if test x$debug = xtrue; then
    AX_CXXFLAGS_GCC_OPTION([-g])
  else
    AX_CXXFLAGS_GCC_OPTION([-fno-rtti])
    AX_CXXFLAGS_GCC_OPTION([-O3])
  fi
fi

# Add debug defines
if test x$debug = xtrue; then
  CPPFLAGS="$CPPFLAGS -DOMEGA_DEBUG"
else
  CPPFLAGS="$CPPFLAGS -DNDEBUG"
fi

# Check the multi-threading flags
OO_MULTI_THREAD

# Check for the builtin intrinsics functions we use
OO_C_BUILTINS

# Check for C++ new functionality
OO_NEW_NOTHROW

# Check for the headers we use
AC_CHECK_HEADERS([windows.h sys/time.h asl.h syslog.h assert.h])

# Add windows resource support to libtool
LT_INIT([win32-dll disable-static])
LT_LANG([Windows Resource])

# Find sqlite3
AX_LIB_SQLITE3([3.5.0])

# Set the WIN32 conditional in the makefiles if the host is MinGW
# If the host is Cygwin, error
case $host_os in
	*cygwin*)
		AC_MSG_ERROR([Don't build for cygwin, use a native Win32 port such as MinGW]) ;;
	*mingw*)
		win32=true ;;
esac
AM_CONDITIONAL([WIN32],[test x$win32 = xtrue])

AC_OUTPUT()