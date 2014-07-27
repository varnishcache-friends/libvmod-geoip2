
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

VMOD_TESTS = tests/*.vtc

$(libvmod_geoip2_la_OBJECTS): vcc_if.h

vcc_if.c vcc_if.h vmod_geoip2.man.rst: $(VMODTOOL) $(top_srcdir)/src/vmod.vcc
	$(PYTHON) $(VMODTOOL) $(top_srcdir)/src/vmod.vcc

vmod_geoip2.3: vmod_geoip2.man.rst
	${RST2MAN} $? $@

check:
	$(VARNISHTEST) -Dvarnishd=$(VARNISHD) \
	    -Dvmod_topbuild=$(abs_top_builddir) $(VMOD_TESTS)

EXTRA_DIST = vmod.vcc \
	$(VMOD_TESTS)

CLEANFILES = $(builddir)/vcc_if.c \
	$(builddir)/vcc_if.h \
	$(builddir)/vmod_geoip2.man.rst \
	$(builddir)/vmod_geoip2.rst
