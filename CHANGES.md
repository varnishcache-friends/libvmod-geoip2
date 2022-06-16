## unreleased

* Drop support for Varnish 6.4, 6.5 and 6.6, and add support for
  7.0 and 7.1.

## 1.2.2 - 2021-04-23

* When we cannot open the DB, send the error to the CLI as well.
* Drop support for Varnish 6.2, 6.3 and 6.4, and add support for
  6.5 and 6.6.

## 1.2.1 - 2019-09-29

* Retire oldstable branch.
* Change master to cover 6.0, 6.2 and 6.3.
* Move master to 6.2.
* Sync with recent changes in varnish master.
* Silence warning when processing the .vcc file.
* Switch Travis CI to xenial and hook codecov.io.
* Add support for returning bytes and strings as JSON.
