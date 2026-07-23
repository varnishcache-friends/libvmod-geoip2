AC_PREREQ([2.69])
AC_INIT([libvmod-geoip2], [devel])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR(src/vmod_geoip2.vcc)
AM_CONFIG_HEADER(config.h)
m4_ifndef([VINYL_VMOD_INCLUDES], AC_MSG_ERROR([vinyl.m4 is required.]))

AM_INIT_AUTOMAKE([1.12 -Wall -Werror foreign color-tests parallel-tests])
AM_SILENT_RULES([yes])

AC_DISABLE_STATIC

AC_PROG_CC_C99
AC_PROG_LIBTOOL

AC_ARG_WITH([lcov],
    AS_HELP_STRING(
        [--with-lcov=PATH],
        [Location of lcov to generate coverage data (auto)]),
    [LCOV="$withval"],
    [AC_CHECK_PROGS(LCOV, [lcov], [])])
AM_CONDITIONAL(HAVE_LCOV, [test -n "$LCOV"])

AC_ARG_WITH([genhtml],
    AS_HELP_STRING(
        [--with-genhtml=PATH],
        [Location of genhtml to generate coverage reports (auto)]),
    [GENHTML="$withval"],
    [AC_CHECK_PROGS(GENHTML, [genhtml], [])])
AM_CONDITIONAL(HAVE_GENHTML, [test -n "$GENHTML"])

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

m4_define_default([_AM_PYTHON_INTERPRETER_LIST],
  [python3.9 python3.8 python3.7 python3.6 python3.5 dnl
   python3.4 python3 python])
AM_PATH_PYTHON([3.4], [], [
  AC_MSG_ERROR([Python 3.4 or later is required.])
])

AC_PATH_PROGS([RST2MAN], [rst2man rst2man.py])
test -z "$RST2MAN" && AC_MSG_ERROR([rst2man is required.])

PKG_CHECK_VAR([VINYL_PREFIX], [vinylapi], [prefix])
PKG_CHECK_VAR([VINYL_BINDIR], [vinylapi], [bindir])
PKG_CHECK_VAR([VINYL_SBINDIR], [vinylapi], [sbindir])
PKG_CHECK_VAR([VINYL_DATAROOT], [vinylapi], [datarootdir])

VINYL_VMOD_INCLUDES
VINYL_VMOD_DIR
VINYL_VMODTOOL

ac_default_prefix=$VINYL_PREFIX

AC_PATH_PROG([VINYLTEST], [vtest], [], [$VINYL_BINDIR:$PATH])
AC_PATH_PROG([VINYLD], [vinyld], [], [$VINYL_SBINDIR:$PATH])

VMOD_TESTS="$(cd $srcdir/src && echo tests/*.vtc)"
AC_SUBST(VMOD_TESTS)

VINYL_PREREQ([9.0.0])
VINYL_VMODS([geoip2])

AM_PROG_AR

LT_PREREQ([2.2.6])
LT_INIT([dlopen disable-static])

AC_CONFIG_FILES([
        Makefile
        src/Makefile
])

AC_CHECK_PROG([HAVE_GIT], [git], [yes], [no])
if test "x$HAVE_GIT" = "xyes"; then
	if git rev-parse --git-dir >/dev/null 2>&1; then
		HAVE_GIT_REPO=yes
	else
		HAVE_GIT_REPO=no
	fi
else
	HAVE_GIT_REPO=no
fi
AM_CONDITIONAL([HAVE_VCS_VMOD_VERSION], [test -f "src/vmod_vcs_version.txt" || test "x$HAVE_GIT_REPO" = "xyes"])
if test "x$HAVE_GIT_REPO" != "xyes" && test ! -f "src/vmod_vcs_version.txt"; then
   AC_MSG_WARN([Building without version control metadata - vmod_vcs_version.txt will be unavailable])
fi

AC_OUTPUT

AS_ECHO("
	==== $PACKAGE_STRING ====

	vinyl:        $VINYL_VERSION
	prefix:       $prefix
	vmoddir:      $vmoddir
	vcldir:       $vcldir
	pkgvcldir:    $pkgvcldir

	compiler:     $CC
	cflags:       $CFLAGS
	ldflags:      $LDFLAGS
")
