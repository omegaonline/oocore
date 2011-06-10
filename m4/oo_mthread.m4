# oo_mthread.m4
#

# OO_MULTI_THREAD
# --------------------------------------

AC_DEFUN([OO_MULTI_THREAD],
  [
	AC_REQUIRE([AC_CANONICAL_HOST])

    case $host_os in
      *mingw*)
	    PTHREAD_CFLAGS=-mthreads
        AC_SUBST(PTHREAD_LIBS)
        AC_SUBST(PTHREAD_CFLAGS)
	    ;;
      *)
        AX_PTHREAD ;;
    esac
  ]
)
