ACLOCAL_AMFLAGS = -I m4 -I ${LIBVARNISHAPI_DATAROOTDIR}/aclocal

SUBDIRS = src

DISTCHECK_CONFIGURE_FLAGS = VMOD_DIR='$${libdir}/varnish/vmods'

dist_doc_DATA = LICENSE

EXTRA_DIST = $(top_srcdir)/libmaxminddb/t/maxmind-db/LICENSE \
	$(top_srcdir)/libmaxminddb/t/maxmind-db/test-data/*.mmdb
