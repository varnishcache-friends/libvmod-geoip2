libvmod-geoip2
==============

![ci](https://github.com/varnishcache-friends/libvmod-geoip2/workflows/ci-main/badge.svg)
[![codecov](https://codecov.io/gh/varnishcache-friends/libvmod-geoip2/branch/devel/graph/badge.svg)](https://codecov.io/gh/varnishcache-friends/libvmod-geoip2)

## About

A Varnish 6.0, 7.3 and 7.4 VMOD to query MaxMind GeoIP2 DB files.

For Varnish master refer to the devel branch.  Older Varnish versions
are no longer supported.

## Requirements

To build this VMOD you will need:

* make
* a C compiler, e.g. GCC or clang
* pkg-config
* python3-docutils or docutils in macOS [1]
* varnish-dev in Debian/Ubuntu, varnish-devel in CentOS/RedHat or
  varnish in macOS [1]
* libmaxminddb-dev in recent Debian/Ubuntu releases, maxminddb in
  macOS [1]. See also https://github.com/maxmind/libmaxminddb

If you are building from Git, you will also need:

* autoconf
* automake
* libtool

You will also need to set `PKG_CONFIG_PATH` to the directory where
**varnishapi.pc** is located before running `autogen.sh` and
`configure`.  For example:

```
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
```

Finally, to use it you will need one or more GeoIP2 or GeoLite2
binary databases.  See https://dev.maxmind.com/.

## Installation

### From a tarball

To install this VMOD, run the following commands:

```
./configure
make
make check
sudo make install
```

The `make check` step is optional but it's good to know whether the
tests are passing on your platform.

### From the Git repository

To install from Git, clone this repository by running:

```
git clone --branch main --recursive https://github.com/varnishcache-friends/libvmod-geoip2
```

And then run `./autogen.sh` followed by the instructions above for
installing from a tarball.

### Packages

See https://github.com/varnishcache-friends/libvmod-geoip2/wiki#packages.

## Example

```
import geoip2;

sub vcl_init {
	new country = geoip2.geoip2("/path/to/GeoLite2-Country.mmdb");
}

sub vcl_recv {
	if (country.lookup("country/names/en", client.ip) != "Japan") {
		...
	}
}
```

More examples available at https://github.com/varnishcache-friends/libvmod-geoip2/wiki.

## DB updates

To update the GeoIP2 DB, download the new file on the same filesystem
as the old one and move it over. See also
https://github.com/maxmind/geoipupdate.

## License

This VMOD is licensed under BSD license. See LICENSE for details.

### Note

1. Using Homebrew, https://github.com/Homebrew/brew/.
