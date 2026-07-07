# iexec

`iexec` is a small process wrapper whose primary purpose is to run as a
Docker-style init process and reap orphaned child processes.

The project should be read as an init/reaper utility first, not as a general
PID namespace management tool.

## Purpose

The intended production use is:

```sh
iexec COMMAND [ARG]...
```

When `iexec` is used as PID 1 in a container, it should:

- start the requested command as the main child process
- reap orphaned descendant processes so zombies do not accumulate
- eventually exit with the same result as the main child process
- forward termination signals to the main child process so container shutdown
  works as expected

When `iexec` is not PID 1, it currently uses `PR_SET_CHILD_SUBREAPER` so it can
still act as a subreaper for orphaned descendants.

## PID Namespace Mode

`--pidns` exists to support validation outside Docker.

The expected use case is to create or enter a PID namespace on a non-Docker
environment so the init/reaper behavior can be tested before relying on it in a
container runtime. This mode is not the main production path.

In other words:

- Docker/container use should normally rely on the inherited container PID
  namespace.
- `--pidns=new`, `--pidns=pid:PID`, `--pidns=file:PATH`, and `--pidns=fd:FD`
  are validation and development aids.
- Privilege handling required for PID namespace setup must be treated as a
  separate risk surface from the ordinary Docker init/reaper path.

## Current Status

Implemented:

- command execution with optional leading `NAME=value` environment assignments
- child reaping through `wait(2)`
- subreaper setup when not running as PID 1
- main child exit status and signal termination propagation
- shutdown signal forwarding to the main child
- non-privileged behavioral tests for the init/reaper contract
- optional PID namespace creation or entry for non-Docker validation
- optional parent-death signal configuration

Known incomplete areas:

- privileged PID namespace behavior is not covered by default tests

See [docs/backlog.md](docs/backlog.md) for the implementation direction.
See [docs/docker.md](docs/docker.md) for Docker entrypoint usage.
See [docs/privilege.md](docs/privilege.md) for privilege and install policy.
See [docs/pidns-validation.md](docs/pidns-validation.md) for `--pidns` scope.
See [docs/install.md](docs/install.md) and [docs/release.md](docs/release.md)
for install and release notes.
See [docs/ci.md](docs/ci.md) for the default CI scope.

## Build

```sh
autoreconf -fi
./configure
make
```

The current build system uses Autotools.
