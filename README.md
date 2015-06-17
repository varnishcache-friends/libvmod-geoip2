libvmod-geoip2
==============

[![Gitter](https://badges.gitter.im/Join Chat.svg)](https://gitter.im/fgsch/libvmod-geoip2?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Build Status](https://travis-ci.org/fgsch/libvmod-geoip2.svg?branch=master)](https://travis-ci.org/fgsch/libvmod-geoip2)

Building/Installing
-------------------

Dependencies:

* build-essential
* pkg-config
* autoconf
* libvarnishapi-dev

```shell
  cd libmaxminddb && ./bootstrap && ./configure && make check && make install && ldconfig
  cd .. && ./autogen.sh && ./configure && make check && make install
```
