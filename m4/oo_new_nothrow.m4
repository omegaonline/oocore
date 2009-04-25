
AC_DEFUN([OO_NEW_NOTHROW],
[
	AC_LANG_PUSH([C++])
	AC_MSG_CHECKING([for new (std::nothrow) T()])
	AC_COMPILE_IFELSE(
        [
			AC_LANG_PROGRAM([[ #include <new> ]],
            [[
				struct s
				{
					int x;
				};
				s* p = new (std::nothrow) s();
            ]]
        )
        ],
        [
			AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE_NEW_NOTHROW], [1], [Define to 1 if you have new (std::nothrow) support])
        ],
        [
			AC_MSG_RESULT([no])
        ]
    )
    
    AC_LANG_POP([C++])
])
