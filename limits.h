#ifndef LIMITS_H
#define LIMITS_H

#define CHAR_BIT    8

#define SCHAR_MIN    (-SCHAR_MAX - 1)
#define SCHAR_MAX    0x7F
#define UCHAR_MAX    0xFF

#ifdef __CHAR_UNSIGNED__
 #define CHAR_MIN    0
 #define CHAR_MAX    UCHAR_MAX
#else
 #define CHAR_MIN    SCHAR_MIN
 #define CHAR_MAX    SCHAR_MAX
#endif

#define SHRT_MIN    (-SHRT_MAX - 1)
#define SHRT_MAX    0x7FFF
#define USHRT_MAX   0xFFFF

#define INT_MIN     (-INT_MAX - 1)
#define INT_MAX     0x7FFFFFFF
#define UINT_MAX    0xFFFFFFFF

#define LONG_MAX    0x7FFFFFFFL
#define LONG_MIN    (-LONG_MAX - 1L)

#define ULONG_MAX   0xFFFFFFFFUL

#ifdef __LONG_LONG_SUPPORTED
 #define LLONG_MAX   0x7FFFFFFFFFFFFFFFLL
 #define LLONG_MIN   (-LLONG_MAX - 1LL)
 #define ULLONG_MAX  0xFFFFFFFFFFFFFFFFULL
#endif

#define SSIZE_MAX   LONG_MAX     // max ssize_t
#define SIZE_T_MAX  ULONG_MAX    // max size_t

#endif
