
AC_DEFUN([OO_CHECK_SOCKOPT],
[
	AC_LANG_PUSH([C])
	m4_foreach_w([oo_check_sockop],[$1],
	[
		AC_MSG_CHECKING([for oo_check_sockop socket option])

		AC_COMPILE_IFELSE(
			[
				AC_LANG_PROGRAM([[ #include <sys/socket.h> ]],
				[
					setsockopt(12, SOL_SOCKET, oo_check_sockop, "nothing", 1);
				]
			)
			],
			[
				AC_MSG_RESULT([yes])
				AC_DEFINE_UNQUOTED([HAVE_]AS_TR_CPP(oo_check_sockop), [1], [Define to 1 if you have the ]oo_check_sockop[ socket option])
			],
			[
				AC_MSG_RESULT([no])
			]
		)
	])
	AC_LANG_POP([C])
])
