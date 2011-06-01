# ===========================================================================
#         http://www.nongnu.org/autoconf-archive/ax_lib_sqlite3.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_LIB_SQLITE3([MINIMUM-VERSION])
#
# DESCRIPTION
#
#   Test for the SQLite 3 library of a particular version (or newer)
#
#   This macro takes only one optional argument, required version of SQLite
#   3 library. If required version is not passed, 3.0.0 is used in the test
#   of existance of SQLite 3.
#
#   If no intallation prefix to the installed SQLite library is given the
#   macro searches under /usr, /usr/local, and /opt.
#
#   This macro calls:
#
#     AC_SUBST(SQLITE3_CFLAGS)
#     AC_SUBST(SQLITE3_LIBS)
#
#   And sets:
#
#     HAVE_SQLITE3
#     HAVE_SQLITE3_AMALGAMATION
#
# LICENSE
#
#   Copyright (c) 2008 Mateusz Loskot <mateusz@loskot.net>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.
#
# MODIFICATIONS
#
#   This file has been modified by Rick Taylor
#
#   1) Change the language to C from C++
#   2) Attempt to find the sqlite3.c amalgamation in same same dir as <sqlite3.h>

AC_DEFUN([AX_LIB_SQLITE3],
[
    AC_ARG_WITH([sqlite3],
        [AC_HELP_STRING(
            [--with-sqlite3=@<:@ARG@:>@],
            [use SQLite3 library @<:@default=yes@:>@, optionally specify the prefix for sqlite3 library]
        )],
        [
        if test "$withval" = "no"; then
            WANT_SQLITE3="no"
        elif test "$withval" = "yes"; then
            WANT_SQLITE3="yes"
            ac_sqlite3_path=""
        else
            WANT_SQLITE3="yes"
            ac_sqlite3_path="$withval"
        fi
        ],
        [WANT_SQLITE3="yes"]
    )

    SQLITE3_CFLAGS=""
    SQLITE3_LIBS=""

    if test "x$WANT_SQLITE3" = "xyes"; then

        ac_sqlite3_header="sqlite3.h"

        sqlite3_version_req=ifelse([$1], [], [3.0.0], [$1])
        sqlite3_version_req_major=`expr $sqlite3_version_req : '\([[0-9]]*\)'`
        sqlite3_version_req_minor=`expr $sqlite3_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
        sqlite3_version_req_micro=`expr $sqlite3_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        if test "x$sqlite3_version_req_micro" = "x" ; then
            sqlite3_version_req_micro="0"
        fi

        sqlite3_version_req_number=`expr $sqlite3_version_req_major \* 1000000 \
                                   \+ $sqlite3_version_req_minor \* 1000 \
                                   \+ $sqlite3_version_req_micro`

		AC_MSG_CHECKING([for SQLite3 library >= $sqlite3_version_req])

		if test "$ac_sqlite3_path" != ""; then
            ac_sqlite3_ldflags="-L$ac_sqlite3_path/lib -lsqlite3"
            ac_sqlite3_header_path="$ac_sqlite3_path/include"
            ac_sqlite3_cppflags="-I$ac_sqlite3_header_path"

            dnl Check for header not in include dir
            if test ! -f "ac_sqlite3_header_path/$ac_sqlite3_header"; then
                if test -f "$ac_sqlite3_path/$ac_sqlite3_header" \
                    && test -r "$ac_sqlite3_path/$ac_sqlite3_header"; then
                    ac_sqlite3_header_path="$ac_sqlite3_path"
                    ac_sqlite3_cppflags="-I$ac_sqlite3_path"
                    ac_sqlite3_ldflags=""
                fi
            fi
        else
			for ac_sqlite3_path_tmp in /usr /usr/local /opt ; do
				if test -f "$ac_sqlite3_path_tmp/include/$ac_sqlite3_header" \
					&& test -r "$ac_sqlite3_path_tmp/include/$ac_sqlite3_header"; then
					ac_sqlite3_header_path="$ac_sqlite3_path_tmp/include"
					ac_sqlite3_cppflags="-I$ac_sqlite3_header_path"
					ac_sqlite3_ldflags="-L$ac_sqlite3_path_tmp/lib -lsqlite3"
					break;
				fi
			done
        fi

        dnl Check for amalgamated version
        if test -f "$ac_sqlite3_header_path/sqlite3.c" \
            && test -r "$ac_sqlite3_header_path/sqlite3.c"; then
            ac_sqlite3_amalgamation=yes
        fi

        saved_CPPFLAGS="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS $ac_sqlite3_cppflags"

        AC_LANG_PUSH(C)
        AC_COMPILE_IFELSE(
            [
            AC_LANG_PROGRAM([[@%:@include <sqlite3.h>]],
                [[
					#if (SQLITE_VERSION_NUMBER >= $sqlite3_version_req_number)
					// Everything is okay
					#else
					#  error SQLite version is too old
					#endif
                ]]
            )
            ],
            [
            AC_MSG_RESULT([yes])
            success="yes"
            ],
            [
            AC_MSG_RESULT([not found])
            success="no"
            ]
        )
        AC_LANG_POP([C])

        CPPFLAGS="$saved_CPPFLAGS"

        if test "$success" = "yes"; then

            SQLITE3_CFLAGS="$ac_sqlite3_cppflags"
            SQLITE3_LIBS="$ac_sqlite3_ldflags"

            dnl Retrieve SQLite release version
            if test "x$ac_sqlite3_header_path" != "x"; then
                ac_sqlite3_version=`cat $ac_sqlite3_header_path/$ac_sqlite3_header \
                    | grep '#define.*SQLITE_VERSION.*\"' | sed -e 's/.* "//' \
                        | sed -e 's/"//'`
                if test $ac_sqlite3_version != ""; then
                    SQLITE3_VERSION=$ac_sqlite3_version
                else
                    AC_MSG_WARN([Can not find SQLITE_VERSION macro in sqlite3.h header to retrieve SQLite version!])
                fi
            fi

            AC_SUBST(SQLITE3_CFLAGS)
            AC_SUBST(SQLITE3_LIBS)
            AC_DEFINE([HAVE_SQLITE3], [1], [Define to 1 if you have the SQLite3 library])

            if test "x$ac_sqlite3_amalgamation" = "xyes"; then
                AC_DEFINE([HAVE_SQLITE3_AMALGAMATION], [1], [Define to 1 if you have the SQLite3 amalgamated sources])
            fi
        fi
    fi
])

AC_DEFUN([OO_LIB_SQLITE3],
[
	sqlite3_version_req=ifelse([$1], [], [3.0.0], [$1])
	
	# Check for pkg-config
	PKG_PROG_PKG_CONFIG
	
	AS_IF([test -n "$PKG_CONFIG"],
		PKG_CHECK_MODULES([SQLITE3],
			[sqlite3 >= $sqlite3_version_req],
			[AC_DEFINE([HAVE_SQLITE3], [1], [Define to 1 if you have the SQLite3 library]) AC_SUBST([SQLITE3_CFLAGS]) AC_SUBST([SQLITE3_LIBS])],[])
	)
	
	AS_IF([test -z "$SQLITE3_CFLAGS"],[AX_LIB_SQLITE3([$1])])
])
