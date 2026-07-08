# Install Notes

## Build From a Checkout

```sh
autoreconf -fi
./configure
make
make check
```

## Default Install

The default install path is for the Docker init/reaper use case. It does not
set Linux file capabilities or the setuid bit:

```sh
./configure
make
make install
```

## Capability Install for PID Namespace Validation

Prefer a capability-based install for deliberate non-Docker PID namespace
validation workflows that need to create or enter PID namespaces:

```sh
./configure --enable-cap-install
make
make install
```

This applies `cap_sys_admin+ep` to the installed `iexec` binary. `CAP_SYS_ADMIN`
is still sensitive, but it is narrower than a setuid-root install.

## Setuid Fallback for PID Namespace Validation

Use setuid only as an explicit fallback when the capability install is not
available or does not work for the target validation host:

```sh
./configure --enable-setuid-install
make
make install
```

`--enable-cap-install` and `--enable-setuid-install` are mutually exclusive.
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

Manual `iexec --pidns=...` validation commands must include
`--allow-privileged-pidns`.

Validate install privilege policy before release:

```sh
sudo -n tests/install-policy.sh
```
