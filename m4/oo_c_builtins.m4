
AC_DEFUN([OO_C_BUILTINS],
[
	AC_LANG_PUSH([C++])
	AC_MSG_CHECKING([for __sync_lock_test_and_set compiler intrinsic])
	AC_COMPILE_IFELSE(
        [
			AC_LANG_PROGRAM([[ ]],
            [[
				long x = 12;
				long y = __sync_lock_test_and_set(&x,57);
            ]]
        )
        ],
        [
			AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE___SYNC_TEST_AND_SET], [1], [Define to 1 if you have the __sync_lock_test_and_set compiler intrinsic])
        ],
        [
			AC_MSG_RESULT([unsupported])
        ]
    )
    
    AC_MSG_CHECKING([for __sync_add_and_fetch compiler intrinsic])
	AC_COMPILE_IFELSE(
        [
			AC_LANG_PROGRAM([[ ]],
            [[
				long x = 12;
				long y = __sync_add_and_fetch(&x,57);
            ]]
        )
        ],
        [
			AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE___SYNC_ADD_AND_FETCH], [1], [Define to 1 if you have the __sync_add_and_fetch compiler intrinsic])
        ],
        [
			AC_MSG_RESULT([unsupported])
        ]
    )
    
    AC_MSG_CHECKING([for __builtin_bswap32 compiler intrinsic])
	AC_COMPILE_IFELSE(
        [
			AC_LANG_PROGRAM([[ ]],
            [[
				long x = 12;
				long y = __builtin_bswap32(x);
            ]]
        )
        ],
        [
			AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE___BUILTIN_BSWAP32], [1], [Define to 1 if you have the __builtin_bswap32 compiler intrinsic])
        ],
        [
			AC_MSG_RESULT([unsupported])
        ]
    )
    
    AC_MSG_CHECKING([for __builtin_bswap64 compiler intrinsic])
	AC_COMPILE_IFELSE(
        [
			AC_LANG_PROGRAM([[ ]],
            [[
				long long x = 12;
				long long y = __builtin_bswap64(x);
            ]]
        )
        ],
        [
			AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE___BUILTIN_BSWAP64], [1], [Define to 1 if you have the __builtin_bswap64 compiler intrinsic])
        ],
        [
			AC_MSG_RESULT([unsupported])
        ]
    )
    
    AC_LANG_POP([C++])
])
