AC_INIT([libvmod-geoip2], [1.1.1])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR(src/vmod.vcc)
AM_CONFIG_HEADER(config.h)
m4_ifndef([VARNISH_VMOD_INCLUDES], AC_MSG_ERROR([varnish.m4 is required.]))

AM_INIT_AUTOMAKE([foreign color-tests parallel-tests])

AC_DISABLE_STATIC

AC_PROG_CC_C99
AC_PROG_LIBTOOL

AC_ARG_ENABLE([gcov],
  [AS_HELP_STRING([--enable-gcov],
     [enable code coverage analysis (default=no)])],
  [AC_PATH_PROG([LCOV], [lcov])
   AC_PATH_PROG([GENHTML], [genhtml])
   CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"])
AM_CONDITIONAL(USE_LCOV, [test x$LCOV != x && test x$GENHTML != x])

PKG_CHECK_EXISTS([libmaxminddb], [
  PKG_CHECK_MODULES([MAXMINDDB], [libmaxminddb])], [
  AC_SEARCH_LIBS([MMDB_open], [maxminddb], [],
    AC_MSG_ERROR([maxminddb is required.]))
])

AM_PATH_PYTHON([2.6], [], [
  AC_MSG_ERROR([Python 2.6 or later is required.])
])

AC_PATH_PROG([RST2MAN], [rst2man])
test -z "$RST2MAN" && AC_MSG_ERROR([rst2man is required.])

PKG_CHECK_VAR([VARNISH_PREFIX], [varnishapi], [prefix])
PKG_CHECK_VAR([VARNISH_BINDIR], [varnishapi], [bindir])
PKG_CHECK_VAR([VARNISH_SBINDIR], [varnishapi], [sbindir])
PKG_CHECK_VAR([VARNISH_DATAROOT], [varnishapi], [datarootdir])

VARNISH_VMOD_INCLUDES
VARNISH_VMOD_DIR
VARNISH_VMODTOOL

ac_default_prefix=$VARNISH_PREFIX

AC_PATH_PROG([VARNISHTEST], [varnishtest], [], [$VARNISH_BINDIR:$PATH])
AC_PATH_PROG([VARNISHD], [varnishd], [], [$VARNISH_SBINDIR:$PATH])

VMOD_TESTS="$(cd $srcdir/src && echo tests/*.vtc)"
AC_SUBST(VMOD_TESTS)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
