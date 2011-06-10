AC_PREREQ(2.61)

AC_INIT(omegaonline,0.5.0)
AC_CONFIG_SRCDIR([src/OOCore/OOCore.cpp])
AC_CONFIG_HEADERS([autoheader_dummy.h src/oocore-autoconf.h])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SUBDIRS([oobase])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([
	Makefile
	include/Makefile
	src/Makefile
	src/OOCore/Makefile
	src/OOServer/Makefile
	data/Makefile
	tools/Makefile
	tools/OOLaunch/Makefile
	tools/OOReg/Makefile
	test/Makefile
	test/TestLibrary/Makefile
	test/TestProcess/Makefile
])

AM_INIT_AUTOMAKE([foreign 1.10 no-define])

# Use the latest short output macro if available
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# I like maintainer mode
AM_MAINTAINER_MODE

AC_CANONICAL_HOST

# Set the WIN32 conditional in the makefiles if the host is MinGW
# If the host is Cygwin, error
AS_CASE([$host_os],
	[*cygwin*],[AC_MSG_ERROR([Don't build for cygwin, use a native Win32 port such as MinGW])],
	[mingw*],[win32=true]
)
AM_CONDITIONAL([WIN32],[test "x$win32" = "xtrue"])

# Add the --enable-debug arg
AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[Turn on debugging]),[debug=true],[debug=false])
AM_CONDITIONAL([DEBUG], [test "x$debug" = "xtrue"])

OO_PROG_CC
OO_PROG_CXX

# Check the multi-threading flags
OO_MULTI_THREAD

# Find sqlite3
OO_LIB_SQLITE3([3.5.0])
AS_IF([test -z "$SQLITE3_LIBS"],[AC_MSG_ERROR([Need a valid sqlite3 library])])

# Check for the headers we use
AC_CHECK_HEADERS([windows.h unistd.h uuid/uuid.h])

# Add the correct libs to LIBS
AC_CHECK_LIB([uuid],[uuid_generate],AC_SUBST([UUID_LIBS],["-luuid"]))

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
    AX_CFLAGS_GCC_OPTION([-O2])
  ])
  
  AX_CFLAGS_GCC_OPTION([-pipe])
    
  AS_CASE([$host_os],[mingw*],
  [
    AX_CFLAGS_GCC_OPTION([-march=i686])
    AX_CFLAGS_GCC_OPTION([-mtune=native])
	
	AX_CFLAGS_GCC_OPTION([-static-libgcc])	
  ])
])

# Set sensible default CXXFLAGS if necessary
AS_IF([test "x$oo_test_CXXFLAGS" != "xset"],
[
  # Always add full warnings
  AX_CXXFLAGS_WARN_ALL

  # Always try to hide symbols
  AX_CXXFLAGS_GCC_OPTION([-fvisibility=hidden])
  AX_CXXFLAGS_GCC_OPTION([-fno-rtti])
  
  AS_IF([test "x$debug" = "xtrue"],
  [
  	AX_CXXFLAGS_GCC_OPTION([-ggdb])
  	AX_CXXFLAGS_GCC_OPTION([-O0])
  ],
  [
    AX_CXXFLAGS_GCC_OPTION([-O2])
  ])
  
  AX_CXXFLAGS_GCC_OPTION([-pipe])
  
  AS_CASE([$host_os],[mingw*],
  [
    AX_CXXFLAGS_GCC_OPTION([-march=i686])
    AX_CXXFLAGS_GCC_OPTION([-mtune=native])
	
	AX_CXXFLAGS_GCC_OPTION([-static-libgcc])
	AX_CXXFLAGS_GCC_OPTION([-static-libstdc++])
  ])
])

# Add debug defines
AS_IF([test "x$debug" = "xtrue"],[CPPFLAGS="$CPPFLAGS -DOMEGA_DEBUG"],[CPPFLAGS="$CPPFLAGS -DNDEBUG"])

# Set up libtool correctly
m4_ifdef([LT_PREREQ],,[AC_MSG_ERROR([Need libtool version 2.2.6 or later])])
LT_PREREQ([2.2.6])
LT_INIT([win32-dll disable-static])
LT_LANG([Windows Resource])

AC_SUBST([lt_cv_prog_compiler_pic_CXX])

AC_PATH_PROG([SQLITE3],[sqlite3],[AC_MSG_ERROR([Need sqlite3 command])])

AC_OUTPUT()
