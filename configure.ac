#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.72])
AC_INIT([cryptochan],[1.0],[dsuchka@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/cryptochan.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

#
## Checks for libraries.
#

# libconfig
PKG_CHECK_MODULES([LIBCONFIG], [libconfig >= 1.5],,
  AC_MSG_ERROR([libconfig 1.5 or newer not found.])
)
AC_SUBST([LIBCONFIG_CFLAGS])
AC_SUBST([LIBCONFIG_LIBS])

# libsecp256k1
PKG_CHECK_MODULES([LIBSECP256K1], [libsecp256k1 >= 0.5],,
  AC_MSG_ERROR([libsecp256k1 0.5 or newer not found.])
)
AC_SUBST([LIBSECP256K1_CFLAGS])
AC_SUBST([LIBSECP256K1_LIBS])

# libbase58
PKG_CHECK_MODULES([LIBBASE58], [libbase58 >= 0.1],,
  AC_MSG_ERROR([libbase58 0.1 or newer not found.])
)
AC_SUBST([LIBBASE58_CFLAGS])
AC_SUBST([LIBBASE58_LIBS])

# combine all together
DEPS_CFLAGS="$LIBCONFIG_CFLAGS $LIBSECP256K1_CFLAGS $LIBBASE58_CFLAGS"
DEPS_LDFLAGS="$LIBCONFIG_LIBS $LIBSECP256K1_LIBS $LIBBASE58_LIBS"

AC_SUBST([DEPS_CFLAGS])
AC_SUBST([DEPS_LDFLAGS])

# Checks for header files.
AC_CHECK_HEADERS([stdatomic.h argp.h secp256k1.h libconfig.h],,
    AC_MSG_ERROR([required header file was not found])
)

# Checks for typedefs, structures, and compiler characteristics.
AX_CHECK_COMPILE_FLAG([-mavx2], [AVX2_CFLAGS="-mavx2"])
AX_CHECK_COMPILE_FLAG([-msse2], [SSE2_CFLAGS="-msse2"])
CFLAGS="$CFLAGS $AVX2_CFLAGS $SSE2_CFLAGS"

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero strdup])

# Enable GNU extensions
AC_DEFINE([_GNU_SOURCE], [], [Enable GNU extensions.])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

