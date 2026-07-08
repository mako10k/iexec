# Docker Entrypoint Usage

`iexec` is intended to be used as a small Docker-style init process:

```dockerfile
COPY iexec /usr/local/bin/iexec
ENTRYPOINT ["/usr/local/bin/iexec"]
CMD ["your-command", "arg1"]
```

In this mode, `iexec` runs as PID 1 inside the container and the requested
command becomes its main child process.

## Expected Behavior

When used as the container entrypoint, `iexec` should:

- forward shutdown signals such as `SIGTERM` to the main child process
- reap orphaned descendant processes so zombies do not accumulate
- exit with the same exit status as the main child when it exits normally
- return shell-compatible statuses for signal termination, such as `143` for
  `SIGTERM`

Docker use should normally rely on Docker's inherited PID namespace. Do not use
`--pidns` for the ordinary container entrypoint path, and do not grant `iexec`
file capabilities or setuid privilege for this path.

## Minimal Example

```dockerfile
FROM busybox:1.36
COPY iexec /usr/local/bin/iexec
ENTRYPOINT ["/usr/local/bin/iexec"]
CMD ["/bin/sh", "-c", "trap 'exit 0' TERM; while true; do sleep 1; done"]
```

Build and run:

```sh
docker build -t iexec-example .
docker run --rm iexec-example /bin/true
docker run --rm iexec-example /bin/sh -c 'exit 7'
```

The first command should exit `0`; the second should exit `7`.

## Docker Integration Test

The default `make check` suite does not require Docker. To run the Docker
entrypoint integration test on a machine with a Docker daemon:

```sh
IEXEC_TEST_DOCKER=1 make check
```

The Docker test builds a temporary BusyBox-based image, copies the built
`iexec` binary into it, and checks command status propagation, shutdown signal
forwarding, and orphaned child reaping through the entrypoint path.

See [privilege.md](privilege.md) for why the Docker path does not require
privileged installation.
