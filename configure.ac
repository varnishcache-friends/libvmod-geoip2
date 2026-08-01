AC_INIT([libvmod-geoip2], [devel])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR(src/vmod_geoip2.vcc)
AC_CONFIG_HEADERS([config.h])
m4_ifndef([VINYL_VMODS], AC_MSG_ERROR([vinyl.m4 is required.]))

AM_INIT_AUTOMAKE([foreign color-tests parallel-tests])

AC_PROG_CC_C99
LT_INIT([disable-static])

AC_ARG_ENABLE([gcov],
  [AS_HELP_STRING([--enable-gcov],
     [enable code coverage analysis (default=no)])],
  [AC_PATH_PROG([GENHTML], [genhtml])
   AC_PATH_PROG([GCOV], [gcov])
   AC_PATH_PROG([LCOV], [lcov])
   CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"])
AM_CONDITIONAL(USE_LCOV,
  [test x$GENHTML != x && test x$GCOV != x && test x$LCOV != x])

PKG_CHECK_EXISTS([libmaxminddb], [
  PKG_CHECK_MODULES([MAXMINDDB], [libmaxminddb])], [
  AC_SEARCH_LIBS([MMDB_open], [maxminddb], [],
    AC_MSG_ERROR([maxminddb is required.]))
])

AC_PATH_PROGS([RST2MAN], [rst2man rst2man.py])
test -z "$RST2MAN" && AC_MSG_ERROR([rst2man is required.])

VINYL_PREREQ([9.0.0])
VINYL_VMODS([geoip2])

VMOD_TESTS="$(cd $srcdir/src && echo tests/*.vtc)"
AC_SUBST(VMOD_TESTS)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT