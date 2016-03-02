
AM_TESTS_ENVIRONMENT = PATH=$(VARNISH_BINDIR):$(VARNISH_SBINDIR):$$PATH

vmoddir = $(VMOD_DIR)
vmod_LTLIBRARIES = libvmod_geoip2.la

libvmod_geoip2_la_CFLAGS = $(VMOD_INCLUDES)
libvmod_geoip2_la_LDFLAGS = -module -export-dynamic -avoid-version -shared \
	$(LIBS)

libvmod_geoip2_la_SOURCES = \
	vmod_geoip2.c
nodist_libvmod_geoip2_la_SOURCES = \
	vcc_if.c \
	vcc_if.h

dist_man_MANS = vmod_geoip2.3

VTC_LOG_COMPILER = $(VARNISHTEST) -v \
	-Dvmod_topbuild=$(abs_top_builddir) -Dvmod_topsrc=$(abs_top_srcdir)
TEST_EXTENSIONS = .vtc
TESTS = @VMOD_TESTS@

$(libvmod_geoip2_la_OBJECTS): vcc_if.h

vcc_if.c vcc_if.h vmod_geoip2.man.rst: $(VMODTOOL) $(top_srcdir)/src/vmod.vcc
	$(PYTHON) $(VMODTOOL) $(top_srcdir)/src/vmod.vcc

vmod_geoip2.3: vmod_geoip2.man.rst
	$(RST2MAN) $? $@

EXTRA_DIST = vmod.vcc \
	$(TESTS)

CLEANFILES = $(builddir)/vcc_if.c \
	$(builddir)/vcc_if.h \
	$(builddir)/vmod_geoip2.man.rst \
	$(builddir)/vmod_geoip2.rst \
	$(builddir)/$(dist_man_MANS)

if USE_LCOV
check-local:
	$(LCOV) -c -d .libs -o .libs/$(PACKAGE).info
	$(GENHTML) -o lcov .libs/$(PACKAGE).info

clean-local:
	rm -rf lcov
endif
