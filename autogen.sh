#!/bin/sh

dataroot=$(pkg-config --variable=datarootdir vinylapi 2>/dev/null)
if [ -z "$dataroot" ] ; then
	cat <<_EOF

No package 'vinylapi' found

Consider adjusting the PKG_CONFIG_PATH environment variable if you
installed software in a non-standard prefix.

_EOF
	exit 1
fi
export VINYLAPI_DATAROOT=${dataroot}
autoreconf -vif -I${dataroot}/aclocal
