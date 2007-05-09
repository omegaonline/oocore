#ifndef OOCORE_TYPE_SIZES_H_INCLUDED_
#define OOCORE_TYPE_SIZES_H_INCLUDED_

#include <limits.h>

// The number of bytes in a short.
# if !defined(OMEGA_SIZEOF_SHORT)
#   if (USHRT_MAX) == 255U
#     define OMEGA_SIZEOF_SHORT 1
#   elif (USHRT_MAX) == 65535U
#     define OMEGA_SIZEOF_SHORT 2
#   elif (USHRT_MAX) == 4294967295U
#     define OMEGA_SIZEOF_SHORT 4
#   elif (USHRT_MAX) == 18446744073709551615U
#     define OMEGA_SIZEOF_SHORT 8
#   else
#     error: unsupported short size, must be updated for this platform!
#   endif /* USHRT_MAX */
# endif /* !defined(OMEGA_SIZEOF_SHORT) */

// The number of bytes in an int.
# if !defined(OMEGA_SIZEOF_INT)
#   if (UINT_MAX) == 65535U
#     define OMEGA_SIZEOF_INT 2
#   elif (UINT_MAX) == 4294967295U
#     define OMEGA_SIZEOF_INT 4
#   elif (UINT_MAX) == 18446744073709551615U
#     define OMEGA_SIZEOF_INT 8
#   else
#     error: unsupported int size, must be updated for this platform!
#   endif /* UINT_MAX */
# endif /* !defined(OMEGA_SIZEOF_INT) */

// The number of bytes in a long.
# if !defined(OMEGA_SIZEOF_LONG)
#   if (ULONG_MAX) == 65535UL
#     define OMEGA_SIZEOF_LONG 2
#   elif ((ULONG_MAX) == 4294967295UL)
#     define OMEGA_SIZEOF_LONG 4
#   elif ((ULONG_MAX) == 18446744073709551615UL)
#     define OMEGA_SIZEOF_LONG 8
#   else
#     error: unsupported long size, must be updated for this platform!
#   endif /* ULONG_MAX */
# endif /* !defined(OMEGA_SIZEOF_LONG) */

// The number of bytes in a long long.
# if !defined(OMEGA_SIZEOF_LONG_LONG)
#   if defined(ULLONG_MAX)
#     if ((ULLONG_MAX) == 4294967295ULL)
#       define OMEGA_SIZEOF_LONG_LONG 4
#     elif ((ULLONG_MAX) == 18446744073709551615ULL)
#       define OMEGA_SIZEOF_LONG_LONG 8
#     endif
#   elif defined(ULONGLONG_MAX)
#     if ((ULONGLONG_MAX) == 4294967295ULL)
#       define OMEGA_SIZEOF_LONG_LONG 4
#     elif ((ULONGLONG_MAX) == 18446744073709551615ULL)
#       define OMEGA_SIZEOF_LONG_LONG 8
#     endif
#   endif
#   // If we can't determine the size of long long, assume it is 8
#   // instead of erroring out.  (Either ULLONG_MAX and ULONGLONG_MAX
#   // may not be supported; or an extended C/C++ dialect may need to
#   // be selected.  If this assumption is wrong, it can be addressed
#   // in the platform-specific config header.
#   if !defined(OMEGA_SIZEOF_LONG_LONG)
#     define OMEGA_SIZEOF_LONG_LONG 8
#   endif
# endif /* !defined(OMEGA_SIZEOF_LONG_LONG) */

#include <float.h>

// The number of bytes in a float.
# ifndef OMEGA_SIZEOF_FLOAT
#   if FLT_MAX_EXP == 128
#     define OMEGA_SIZEOF_FLOAT 4
#   elif FLT_MAX_EXP == 1024
#     define OMEGA_SIZEOF_FLOAT 8
#   else
#     error: unsupported float size, must be updated for this platform!
#   endif /* FLT_MAX_EXP */
# endif /* OMEGA_SIZEOF_FLOAT */

// The number of bytes in a double.
# ifndef OMEGA_SIZEOF_DOUBLE
#   if DBL_MAX_EXP == 128
#     define OMEGA_SIZEOF_DOUBLE 4
#   elif DBL_MAX_EXP == 1024
#     define OMEGA_SIZEOF_DOUBLE 8
#   else
#     error: unsupported double size, must be updated for this platform!
#   endif /* DBL_MAX_EXP */
# endif /* OMEGA_SIZEOF_DOUBLE */

// Byte-order (endian-ness) determination.
# if defined(BYTE_ORDER)
#   if (BYTE_ORDER == LITTLE_ENDIAN)
#     define OMEGA_LITTLE_ENDIAN 0x0123
#     define OMEGA_BYTE_ORDER OMEGA_LITTLE_ENDIAN
#   elif (BYTE_ORDER == BIG_ENDIAN)
#     define OMEGA_BIG_ENDIAN 0x3210
#     define OMEGA_BYTE_ORDER OMEGA_BIG_ENDIAN
#   else
#     error: unknown BYTE_ORDER!
#   endif /* BYTE_ORDER */
# elif defined(_BYTE_ORDER)
#   if (_BYTE_ORDER == _LITTLE_ENDIAN)
#     define OMEGA_LITTLE_ENDIAN 0x0123
#     define OMEGA_BYTE_ORDER OMEGA_LITTLE_ENDIAN
#   elif (_BYTE_ORDER == _BIG_ENDIAN)
#     define OMEGA_BIG_ENDIAN 0x3210
#     define OMEGA_BYTE_ORDER OMEGA_BIG_ENDIAN
#   else
#     error: unknown _BYTE_ORDER!
#   endif /* _BYTE_ORDER */
# elif defined(__BYTE_ORDER)
#   if (__BYTE_ORDER == __LITTLE_ENDIAN)
#     define OMEGA_LITTLE_ENDIAN 0x0123
#     define OMEGA_BYTE_ORDER OMEGA_LITTLE_ENDIAN
#   elif (__BYTE_ORDER == __BIG_ENDIAN)
#     define OMEGA_BIG_ENDIAN 0x3210
#     define OMEGA_BYTE_ORDER OMEGA_BIG_ENDIAN
#   else
#     error: unknown __BYTE_ORDER!
#   endif /* __BYTE_ORDER */
# else /* ! BYTE_ORDER && ! __BYTE_ORDER */
  // We weren't explicitly told, so we have to figure it out . . .
#   if defined(i386) || defined(__i386__) || defined(_M_IX86) || \
     defined(vax) || defined(__alpha) || defined(__LITTLE_ENDIAN__) ||\
     defined(ARM) || defined(_M_IA64) || \
     defined(_M_AMD64) || defined(__amd64)
    // We know these are little endian.
#     define OMEGA_LITTLE_ENDIAN 0x0123
#     define OMEGA_BYTE_ORDER OMEGA_LITTLE_ENDIAN
#   else
    // Otherwise, we assume big endian.
#     define OMEGA_BIG_ENDIAN 0x3210
#     define OMEGA_BYTE_ORDER OMEGA_BIG_ENDIAN
#   endif
# endif /* ! BYTE_ORDER && ! __BYTE_ORDER */

#endif // OOCORE_TYPE_SIZES_H_INCLUDED_
