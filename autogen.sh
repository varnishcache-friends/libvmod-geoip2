#!/bin/sh

aclocal -I m4
libtoolize --copy --force
autoheader
automake --add-missing --copy --foreign
autoconf
