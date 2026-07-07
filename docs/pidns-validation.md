# PID Namespace Validation Mode

`--pidns` is a validation aid for non-Docker environments.

The production-oriented Docker path should normally run `iexec` in the PID
namespace created by the container runtime. In that path, `iexec` does not need
to create or enter a PID namespace itself.

## Intended Use

Use `--pidns` when you want to exercise init/reaper behavior before or outside a
Docker runtime:

```sh
iexec --pidns=new COMMAND [ARG]...
iexec --pidns=pid:PID COMMAND [ARG]...
iexec --pidns=file:/proc/PID/ns/pid COMMAND [ARG]...
iexec --pidns=fd:FD COMMAND [ARG]...
```

This mode exists so PID 1 behavior, `/proc` preparation, and namespace entry can
be inspected independently from Docker.

## Non-Goals

`iexec` should not be treated as:

- a general-purpose PID namespace administration tool
- a replacement for container runtime namespace management
- a reason to use setuid in the ordinary Docker entrypoint path

## Privilege Boundary

Creating or entering PID namespaces may require elevated privileges. Keep that
separate from the Docker init/reaper path:

- default builds and installs should remain non-setuid
- setuid install is opt-in via `./configure --enable-setuid-install`
- privileged namespace tests should remain opt-in

See [privilege.md](privilege.md) for install policy.

## Test

The default `make check` path skips privileged PID namespace validation. To run
it manually:

```sh
sudo -n env IEXEC_TEST_PIDNS=1 IEXEC_TEST_BINARY="$PWD/src/iexec" \
  tests/pidns-validation.sh
```

The test covers `--pidns=new` and entering an existing PID namespace through
`pid:`, `file:`, and `fd:` selectors.
