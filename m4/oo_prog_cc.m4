# oo_prog_cc
#
# Override AC_PROG_CC and AC_PROG_CXX to make sure autoconf doesn't add useless CFLAG options

# OO_PROG_CC
# --------------------------------------

AC_DEFUN([OO_PROG_CC],
  [
	AC_BEFORE([$0], [AC_PROG_CC])dnl
	
    # Stash the passed in CFLAGS if any...
    oo_test_CFLAGS=${CFLAGS+set}
    oo_save_CFLAGS=$CFLAGS

    # Call the vanilla AC_PROG_CC
    AC_PROG_CC

    # Restore any original CFLAGS if necessary
    if test "$oo_test_CFLAGS" = set; then
      CFLAGS=$oo_save_CFLAGS
    else
      CFLAGS=
    fi
  ]
)

# OO_PROG_CXX
# --------------------------------------

AC_DEFUN([OO_PROG_CXX],
  [
	AC_BEFORE([$0], [AC_PROG_CXX])dnl
	
    # Stash the passed in CXXFLAGS if any...
    oo_test_CXXFLAGS=${CXXFLAGS+set}
    oo_save_CXXFLAGS=$CXXFLAGS

    # Call the vanilla AC_PROG_CXX
    AC_PROG_CXX

    # Restore any original CXXFLAGS if necessary
    if test "$oo_test_CXXFLAGS" = set; then
      CXXFLAGS=$oo_save_CXXFLAGS
    else
      CXXFLAGS=
    fi
  ]
)
