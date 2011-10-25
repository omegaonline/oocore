
AC_DEFUN([OO_LIB_SQLITE3],
[
	sqlite3_version_req=ifelse([$1], [], [3.0.0], [$1])

	PKG_PROG_PKG_CONFIG

	AS_IF([test -n "$PKG_CONFIG"],[PKG_CHECK_MODULES([SQLITE3],[sqlite3 >= $sqlite3_version_req],
		[
			AC_DEFINE([HAVE_SQLITE3_H], [1], [Define to 1 if you have the SQLite3 library])
			AC_SUBST([SQLITE3_CFLAGS])
			AC_SUBST([SQLITE3_LIBS])
		])
	])

	AS_IF([test -z "$SQLITE3_LIBS"],[
		AC_CHECK_LIB([sqlite3],[sqlite3_libversion],AC_SUBST([SQLITE3_LIBS],["-lsqlite3"]))
		AC_CHECK_HEADER([sqlite3.h],
			[
				AC_MSG_CHECKING([for SQLite3 version >= $sqlite3_version_req])

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

				AC_LANG_PUSH(C)
				AC_COMPILE_IFELSE(
					[
					AC_LANG_PROGRAM([[@%:@include <sqlite3.h>]],
						[[
							#if !defined(SQLITE_VERSION_NUMBER) || (SQLITE_VERSION_NUMBER < $sqlite3_version_req_number)
							#error SQLite version is too old
							#endif
						]]
					)],
					[
						AC_MSG_RESULT([yes])
						AC_DEFINE([HAVE_SQLITE3_H], [1], [Define to 1 if you have the SQLite3 library])
					],
					[AC_MSG_RESULT([no])]
				)
				AC_LANG_POP([C])
			])
	])
])
