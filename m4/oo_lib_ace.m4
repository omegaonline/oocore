
AC_DEFUN([OO_LIB_ACE],
[
    AC_ARG_WITH([ace],
        AC_HELP_STRING(
            [--with-ace=@<:@ARG@:>@],
            [use ACE library @<:@default=yes@:>@, optionally specify the prefix for ACE library]
        ),
        [
        if test "$withval" = "no"; then
            WANT_ACE="no"
        elif test "$withval" = "yes"; then
            WANT_ACE="yes"
            ac_ace_path=""
        else
            WANT_ACE="yes"
            ac_ace_path="$withval"
        fi
        ],
        [WANT_ACE="yes"]
    )

    ACE_CPPFLAGS=""
    ACE_LDFLAGS=""
    
    ac_found_ace=no
    
    if test "x$WANT_ACE" = "xyes"; then

        ac_ace_header="ace/ACE.h"

        ace_version_req=ifelse([$1], [], [5.6.0], [$1])
        ace_version_req_major=`expr $ace_version_req : '\([[0-9]]*\)'`
        ace_version_req_minor=`expr $ace_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
        ace_version_req_micro=`expr $ace_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        if test "x$ace_version_req_micro" = "x" ; then
            ace_version_req_micro="0"
        fi

        ace_version_req_number=`expr $ace_version_req_major \* 10000 \
                                   \+ $ace_version_req_minor \* 100 \
                                   \+ $ace_version_req_micro`

        AC_MSG_CHECKING([for ACE library >= $ace_version_req])

        if test "$ac_ace_path" != ""; then
            ac_ace_ldflags="-L$ac_ace_path/lib"
            ac_ace_header_path="$ac_ace_path"
            ac_ace_cppflags="-I$ac_ace_header_path"
        else
            for ac_ace_path_tmp in $ACE_ROOT /usr /usr/local /opt ; do
                if test -f "$ac_ace_path_tmp/include/$ac_ace_header" \
                    && test -r "$ac_ace_path_tmp/include/$ac_ace_header"; then
                    ac_ace_header_path="$ac_ace_path_tmp/include"
                    ac_ace_cppflags="-I$ac_ace_header_path"
                    ac_ace_ldflags="-L$ac_ace_path_tmp/lib"
                    break;
                fi
            done
        fi
        
        ac_ace_ldflags="$ac_ace_ldflags -lACE"
		
        saved_CPPFLAGS="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS $ac_ace_cppflags"

        AC_LANG_PUSH(C++)
        AC_COMPILE_IFELSE(
            [
            AC_LANG_PROGRAM([[@%:@include <ace/ACE.h>]],
                [[
#define TEST_ACE_VERSION(x,y,z) ((x * 10000) + (y * 100) + z)
#define TEST_ACE_VERSION_CURRENT TEST_ACE_VERSION(ACE_MAJOR_VERSION,ACE_MINOR_VERSION,ACE_BETA_VERSION)

#if (TEST_ACE_VERSION_CURRENT >= $ace_version_req_number)
// Everything is okay
#else
#  error ACE version is too old
#endif
                ]]
            )
            ],
            [
            AC_MSG_RESULT([yes])
            ac_found_ace="yes"
            ],
            [
            AC_MSG_RESULT([not found])
            ac_found_ace="no"
            ]
        )
        AC_LANG_POP([C++])

        CPPFLAGS="$saved_CPPFLAGS"
    fi
    
    if test "$ac_found_ace" = "yes"; then

        ACE_CPPFLAGS="$ac_ace_cppflags"
        ACE_LDFLAGS="$ac_ace_ldflags"

        AC_SUBST(ACE_CPPFLAGS)
        AC_SUBST(ACE_LDFLAGS)
    else
        AC_MSG_ERROR([ACE is required])
    fi
])
