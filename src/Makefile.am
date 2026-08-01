AM_CFLAGS = $(VINYLAPI_CFLAGS) $(MAXMINDDB_CFLAGS)

vmod_LTLIBRARIES = libvmod_geoip2.la

libvmod_geoip2_la_LDFLAGS = $(VMOD_LDFLAGS) $(MAXMINDDB_LIBS)

libvmod_geoip2_la_SOURCES = \
	vmod_geoip2.c
nodist_libvmod_geoip2_la_SOURCES = \
	vcc_geoip2_if.c \
	vcc_geoip2_if.h

dist_man_MANS = vmod_geoip2.3

@BUILD_VMOD_GEOIP2@

AM_TESTS_ENVIRONMENT = \
	PATH="$(abs_builddir):$(VINYL_TEST_PATH):$(PATH)" \
	LD_LIBRARY_PATH="$(VINYL_LIBRARY_PATH)"
TEST_EXTENSIONS = .vtc
VTC_LOG_COMPILER = vinyltest -v
AM_VTC_LOG_FLAGS = \
	-Dvmod_topbuild=$(abs_top_builddir) -Dvmod_topsrc=$(abs_top_srcdir)

TESTS = @VMOD_TESTS@

EXTRA_DIST = vmod_geoip2.vcc \
	tests/libvmod-geoip2.mmdb \
	$(TESTS)

DISTCLEANFILES = vmod_vcs_version.txt

if USE_LCOV
check-local:
	$(LCOV) -c -d .libs -o .libs/$(PACKAGE).info
	$(GENHTML) -o lcov .libs/$(PACKAGE).info
	$(GCOV) -o .libs/libvmod_geoip2_la-vcc_geoip2_if.gcno vmod_geoip2.c

clean-local:
	rm -rf lcov
endif