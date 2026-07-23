AM_CFLAGS = $(VINYLAPI_CFLAGS)

# Modules

vmod_LTLIBRARIES = \
	libvmod_geoip2.la

libvmod_geoip2_la_CFLAGS = \
	$(VMOD_INCLUDES) $(MAXMINDDB_CFLAGS)

libvmod_geoip2_la_LDFLAGS = -module -export-dynamic -avoid-version -shared \
	$(LIBS) $(MAXMINDDB_LIBS)

libvmod_geoip2_la_SOURCES = \
	vmod_geoip2.c

nodist_libvmod_geoip2_la_SOURCES = \
	vcc_geoip2_if.c \
	vcc_geoip2_if.h

vmod_vcs_version.txt: $(nodist_libvmod_geoip2_la_SOURCES)

DISTCLEANFILES = vmod_vcs_version.txt

@BUILD_VMOD_GEOIP2@

# Test suite

AM_TESTS_ENVIRONMENT = \
	PATH="$(abs_builddir):$(VINYL_TEST_PATH):$(PATH)" \
	LD_LIBRARY_PATH="$(VINYL_LIBRARY_PATH)"
TEST_EXTENSIONS = .vtc
VTC_LOG_COMPILER = vinyltest -v -Dvmod_topbuild=$(abs_top_builddir) -Dvmod_topsrc=$(abs_top_srcdir)
AM_VTC_LOG_FLAGS = \
	-p vcl_path="$(abs_top_srcdir)/vcl:$(VINYLAPI_VCLDIR)" \
	-p vmod_path="$(abs_builddir)/.libs:$(vmoddir):$(VINYLAPI_VMODDIR)"

TESTS = @VMOD_TESTS@

# Documentation

dist_doc_DATA = vmod_geoip2.vcc \
	tests/libvmod-geoip2.mmdb \
	$(TESTS)

if HAVE_VCS_VMOD_VERSION
dist_doc_DATA += vmod_vcs_version.txt
endif

dist_man_MANS = \
	vmod_geoip2.3

.rst.1:
	$(AM_V_GEN) $(RST2MAN) $< $@
