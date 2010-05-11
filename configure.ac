AC_PREREQ(2.61)

AC_INIT(omegaonline,0.5.0)
AC_CONFIG_SRCDIR([src/OOCore/OOCore.cpp])
AC_CONFIG_HEADER([src/OOBase/autoheader_dummy.h src/OOBase/config-autoconf.h])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([
	Makefile
	include/Makefile
	src/Makefile
	src/OOBase/Makefile
	src/OOCore/Makefile
	src/OOServer/Makefile
	src/OORegister/Makefile
	data/Makefile
	test/Makefile
	test/CoreTests/Makefile
	test/CoreTests/TestLibrary/Makefile
	test/CoreTests/TestProcess/Makefile
])

AM_INIT_AUTOMAKE([foreign 1.10 no-define])

# Use the latest short output macro if available
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

## And color-tests as above
m4_ifdef([AM_COLOR_TESTS], [AM_COLOR_TESTS([yes])])

# I like maintainer mode
AM_MAINTAINER_MODE

AC_CANONICAL_HOST

# Add the --enable-debug arg
AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[Turn on debugging]),[debug=true],[debug=false])
AM_CONDITIONAL([DEBUG], [test "x$debug" = "xtrue"])

OO_PROG_CC
OO_PROG_CXX

# Check for pkg-config
PKG_PROG_PKG_CONFIG

# Check the multi-threading flags
OO_MULTI_THREAD

# Check for the builtin intrinsics functions we use
OO_C_BUILTINS

# Check for C++ new functionality
OO_NEW_NOTHROW

# Check for the headers we use
AC_CHECK_HEADERS([windows.h wchar.h sys/time.h asl.h syslog.h assert.h time.h errno.h sys/socket.h uuid/uuid.h pwd.h unistd.h ev.h fcntl.h sys/fcntl.h sys/un.h ])

# Add the correct libs to LIBS
AC_CHECK_LIB([uuid],[uuid_generate],AC_SUBST([UUID_LIBS],["-luuid"]))
AC_CHECK_LIB([ev],[ev_loop_new],AC_SUBST([EV_LIBS],["-lev"]))

# Check for socket options
OO_CHECK_SOCKOPT([SO_PEERCRED LOCAL_CREDS])

# Check for password functions
AC_CHECK_FUNCS([getpwuid_r])

# Check for specifics of Unix cred passing
AC_CHECK_FUNCS([getpeereid getpeerucred])
AC_CHECK_TYPES([struct cmsgcred])
AC_CHECK_TYPES([struct fcred])
AC_CHECK_TYPES([struct sockcred])

# Check for wcstoull and friends
AC_CHECK_FUNCS([_wcstoui64 _wcstoi64 wcstoull wcstoll])

# Set sensible default CFLAGS if necessary
AS_IF([test "x$oo_test_CFLAGS" != "xset"],
[
  # Always try to hide symbols
  AX_CFLAGS_GCC_OPTION([-fvisibility=hidden])

  AS_IF([test "x$debug" = "xtrue"],
  [
    AX_CFLAGS_GCC_OPTION([-ggdb])
    AX_CFLAGS_GCC_OPTION([-O0])
  ],
  [
    AX_CFLAGS_GCC_OPTION([-O3])
  ])
])

# Set sensible default CXXFLAGS if necessary
AS_IF([test "x$oo_test_CXXFLAGS" != "xset"],
[
  # Always add full warnings
  AX_CXXFLAGS_WARN_ALL

  # Always try to hide symbols
  AX_CXXFLAGS_GCC_OPTION([-fvisibility=hidden])

  AS_IF([test "x$debug" = "xtrue"],
  [
  	AX_CXXFLAGS_GCC_OPTION([-ggdb])
  	AX_CXXFLAGS_GCC_OPTION([-O0])
  ],
  [
    AX_CXXFLAGS_GCC_OPTION([-fno-rtti])
    AX_CXXFLAGS_GCC_OPTION([-O3])
  ])
])

# Add debug defines
AS_IF([test "x$debug" = "xtrue"],[CPPFLAGS="$CPPFLAGS -DOMEGA_DEBUG"],[CPPFLAGS="$CPPFLAGS -DNDEBUG"])

# Set up libtool correctly
m4_ifdef([LT_PREREQ],,[AC_MSG_ERROR([Need libtool version 2.2.6 or later])])
LT_PREREQ([2.2.6])
LT_CONFIG_LTDL_DIR([libltdl])
LT_INIT([win32-dll disable-static dlopen])
LT_LANG([Windows Resource])
LTDL_INIT

# Find sqlite3
OO_LIB_SQLITE3([3.5.0])
AC_PATH_PROG([SQLITE3],[sqlite3],[AC_MSG_ERROR([Need squlite3 command])])

# Set the WIN32 conditional in the makefiles if the host is MinGW
# If the host is Cygwin, error
AS_CASE([$host_os],
	[*cygwin*],[AC_MSG_ERROR([Don't build for cygwin, use a native Win32 port such as MinGW])],
	[mingw*],[win32=true]
)
AM_CONDITIONAL([WIN32],[test "x$win32" = "xtrue"])

AC_OUTPUT()
