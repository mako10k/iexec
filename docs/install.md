# Install Notes

## Build From a Checkout

```sh
autoreconf -fi
./configure
make
make check
```

## Default Install

The default install path is for the Docker init/reaper use case and does not set
the setuid bit:

```sh
./configure
make
make install
```

## Setuid Install for PID Namespace Validation

Only enable setuid install for a deliberate non-Docker PID namespace validation
workflow:

```sh
./configure --enable-setuid-install
make
make install
```

This is not required for ordinary Docker entrypoint use. See
[privilege.md](privilege.md) and [pidns-validation.md](pidns-validation.md).

## Optional Docker Integration Test

The default test suite does not require Docker:

```sh
make check
```

Run the Docker entrypoint integration test only on a host with a working Docker
daemon:

```sh
IEXEC_TEST_DOCKER=1 make check
```

Run privileged PID namespace validation only on a host where root may create and
enter PID namespaces:

```sh
sudo -n env IEXEC_TEST_PIDNS=1 IEXEC_TEST_BINARY="$PWD/src/iexec" \
  tests/pidns-validation.sh
```
