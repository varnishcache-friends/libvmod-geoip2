#!/bin/sh

dataroot=$(
	pkg-config --variable=datarootdir vinylapi 2>/dev/null ||
	pkg-config --variable=datarootdir varnishapi 2>/dev/null
)
if [ -z "$dataroot" ] ; then
	cat <<_EOF

No package 'vinylapi/varnishapi' found

Consider adjusting the PKG_CONFIG_PATH environment variable if you
installed software in a non-standard prefix.

_EOF
	exit 1
fi
export VCACHEAPI_DATAROOTDIR=${dataroot}
autoreconf -vif -I${dataroot}/aclocal
